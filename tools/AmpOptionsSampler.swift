import Foundation
import CoreMIDI

guard CommandLine.arguments.count == 2 else { fatalError("Usage: swift tools/AmpOptionsSampler.swift OUTPUT_DIRECTORY") }
let outputDirectory = URL(fileURLWithPath: CommandLine.arguments[1], isDirectory: true)
try FileManager.default.createDirectory(at: outputDirectory, withIntermediateDirectories: true)
var client = MIDIClientRef(), input = MIDIPortRef(), output = MIDIPortRef()
var destination = MIDIEndpointRef()
let lock = NSLock()
var pendingBytes = [UInt8]()
var messages = [[UInt8]]()
let ready = DispatchSemaphore(value: 0)
MIDIClientCreateWithBlock("BSII controlled sampler" as CFString, &client, nil)
MIDIOutputPortCreate(client, "sampler out" as CFString, &output)
MIDIInputPortCreateWithBlock(client, "sampler in" as CFString, &input) { list, _ in
    // Walk packet pointers within the original list, never a copied packet.
    let packetOffset = MemoryLayout<MIDIPacketList>.offset(of: \.packet)!
    let first = UnsafeRawPointer(list).advanced(by: packetOffset).assumingMemoryBound(to: MIDIPacket.self)
    do {
        var packet = first
        for _ in 0..<list.pointee.numPackets {
            let bytes = withUnsafeBytes(of: packet.pointee.data) { Array($0.prefix(Int(packet.pointee.length))) }
            lock.lock()
            for byte in bytes {
                if byte == 0xF0 { pendingBytes = [byte] }
                else if !pendingBytes.isEmpty {
                    if byte >= 0xF8 { continue }
                    pendingBytes.append(byte)
                    if byte == 0xF7 {
                        if pendingBytes.count == 154 && Array(pendingBytes.prefix(6)) == [0xF0,0,0x20,0x29,0,0x33] {
                            messages.append(pendingBytes); ready.signal()
                        }
                        pendingBytes.removeAll()
                    }
                }
            }
            lock.unlock()
            packet = UnsafePointer(MIDIPacketNext(packet))
        }
    }
}
func name(_ object: MIDIObjectRef) -> String {
    var value: Unmanaged<CFString>?
    MIDIObjectGetStringProperty(object, kMIDIPropertyDisplayName, &value)
    return value?.takeRetainedValue() as String? ?? ""
}
var sources = [MIDIEndpointRef](), destinations = [MIDIEndpointRef]()
for i in 0..<MIDIGetNumberOfSources() { let s=MIDIGetSource(i); if name(s).lowercased().contains("bass station") { sources.append(s) } }
for i in 0..<MIDIGetNumberOfDestinations() { let d=MIDIGetDestination(i); if name(d).lowercased().contains("bass station") { destinations.append(d) } }
guard sources.count == 1 && destinations.count == 1 else { fatalError("Expected exactly one BSII input and output") }
destination = destinations[0]
MIDIPortConnectSource(input, sources[0], nil)
func send(_ bytes: [UInt8]) {
    let storage = UnsafeMutableRawPointer.allocate(byteCount: 2048, alignment: 8)
    defer { storage.deallocate() }
    let list = storage.bindMemory(to: MIDIPacketList.self, capacity: 1)
    let first = MIDIPacketListInit(list)
    bytes.withUnsafeBufferPointer { data in
        _ = MIDIPacketListAdd(list, 2048, first, 0, data.count, data.baseAddress!)
    }
    precondition(MIDISend(output, destination, list) == noErr)
}
func patch() -> [UInt8]? {
    while ready.wait(timeout: .now()) == .success {}
    lock.lock(); messages.removeAll(); lock.unlock()
    send([0xF0,0,0x20,0x29,0,0x33,0,0x40,0xF7])
    guard ready.wait(timeout: .now()+2) == .success else { return nil }
    lock.lock(); defer { lock.unlock() }
    return messages.last
}
func nrpn(_ number: UInt8, _ value: UInt8) {
    send([0xB0,99,0]); Thread.sleep(forTimeInterval: 0.005)
    send([0xB0,98,number]); Thread.sleep(forTimeInterval: 0.005)
    send([0xB0,6,value]); Thread.sleep(forTimeInterval: 0.1)
}
guard let baseline = patch() else { fatalError("No baseline; nothing changed") }
try Data(baseline).write(to: outputDirectory.appendingPathComponent("before.syx"))
var rows = ["fixed_sent,count_sent,changed_bytes"]
var count = 0
for fixed in 0...1 {
    for countValue in 0...16 {
        nrpn(114,UInt8(fixed)); nrpn(117,UInt8(countValue))
        guard let result = patch() else { continue }
        let changes = zip(baseline,result).enumerated().filter { $0.element.0 != $0.element.1 }.map { "\($0.offset):\($0.element.0)>\($0.element.1)" }.joined(separator:";")
        rows.append("\(fixed),\(countValue),\(changes)")
        try Data(result).write(to: outputDirectory.appendingPathComponent("fixed-\(fixed)-count-\(countValue).syx"))
        print(rows.last!); fflush(stdout); count += 1
    }
}
send(baseline); Thread.sleep(forTimeInterval: 0.3)
let restored = patch()
if let restored = restored { try Data(restored).write(to: outputDirectory.appendingPathComponent("after.syx")) }
try (rows.joined(separator:"\n")+"\n").write(to: outputDirectory.appendingPathComponent("amp-options.csv"),atomically:true,encoding:.utf8)
print("Captured \(count) cases; original patch restored: \(restored == baseline)")
MIDIPortDispose(input); MIDIPortDispose(output); MIDIClientDispose(client)
exit(count == 34 && restored == baseline ? 0 : 1)
