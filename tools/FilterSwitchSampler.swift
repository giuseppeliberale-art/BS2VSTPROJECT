import Foundation
import CoreMIDI

guard CommandLine.arguments.count == 2 else { fatalError("Usage: swift tools/FilterSwitchSampler.swift OUTPUT_DIRECTORY") }
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
func cc(_ number: UInt8, _ value: UInt8) {
    send([0xB0,number,value]); Thread.sleep(forTimeInterval: 0.1)
}
guard let baseline = patch() else { fatalError("No baseline; no parameters changed") }
try Data(baseline).write(to: outputDirectory.appendingPathComponent("before.syx"))
var rows = ["type_sent,shape_sent,slope_sent,type_raw,shape_raw,slope_raw,effective_shape,effective_slope,match"]
var failures = 0
for type in 0...1 {
    for shape in 0...2 {
        for slope in 0...1 {
            cc(83,0); cc(84,UInt8(shape)); cc(106,UInt8(slope)); cc(83,UInt8(type))
            guard let result = patch() else { failures += 1; continue }
            let actualType = (Int(result[48]) & 4) >> 2
            let actualShape = Int(result[48]) & 3
            let actualSlope = (Int(result[48]) & 8) >> 3
            let effectiveShape = actualType == 1 ? 0 : actualShape
            let effectiveSlope = actualType == 1 ? 1 : actualSlope
            let expectedShape = type == 1 ? 0 : shape
            let expectedSlope = type == 1 ? 1 : slope
            let ok = actualType == type && effectiveShape == expectedShape && effectiveSlope == expectedSlope
            if !ok { failures += 1 }
            rows.append("\(type),\(shape),\(slope),\(actualType),\(actualShape),\(actualSlope),\(effectiveShape),\(effectiveSlope),\(ok)")
            try Data(result).write(to: outputDirectory.appendingPathComponent("type-\(type)-shape-\(shape)-slope-\(slope).syx"))
            print(rows.last!); fflush(stdout)
        }
    }
}
send(baseline); Thread.sleep(forTimeInterval: 0.3)
let restored = patch()
let restoreOK = restored == baseline
if let restored = restored { try Data(restored).write(to: outputDirectory.appendingPathComponent("after.syx")) }
try (rows.joined(separator:"\n")+"\n").write(to: outputDirectory.appendingPathComponent("filter-switches.csv"),atomically:true,encoding:.utf8)
print("Cases: \(rows.count-1); failures: \(failures); entire starting patch restored: \(restoreOK)")
MIDIPortDispose(input); MIDIPortDispose(output); MIDIClientDispose(client)
exit(failures == 0 && restoreOK && rows.count == 13 ? 0 : 1)
