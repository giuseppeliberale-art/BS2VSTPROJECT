import Foundation
import CoreMIDI

guard CommandLine.arguments.count == 3 else { fatalError("Usage: swift tools/PatchCheckpoint.swift OUTPUT_DIRECTORY capture|compare|restore") }
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
defer { MIDIPortDispose(input); MIDIPortDispose(output); MIDIClientDispose(client) }
let mode = CommandLine.arguments[2]
let baselineURL = outputDirectory.appendingPathComponent("baseline.syx")
if mode == "capture" {
    guard !FileManager.default.fileExists(atPath: baselineURL.path) else { fatalError("Baseline exists; refusing to overwrite") }
    guard let bytes = patch() else { fatalError("No patch response; nothing changed") }
    try Data(bytes).write(to: baselineURL)
    print("Saved original 154-byte patch")
} else if mode == "compare" || mode == "restore" {
    let baseline = Array(try Data(contentsOf: baselineURL))
    guard baseline.count == 154 && Array(baseline.prefix(6)) == [0xF0,0,0x20,0x29,0,0x33] && baseline.last == 0xF7 else { fatalError("Invalid baseline") }
    if mode == "restore" { send(baseline); Thread.sleep(forTimeInterval: 0.3) }
    guard let current = patch() else { fatalError("No patch response") }
    try Data(current).write(to: outputDirectory.appendingPathComponent(mode + "-readback.syx"))
    let differences = zip(baseline,current).enumerated().filter { $0.element.0 != $0.element.1 }.map { "\($0.offset):\($0.element.0)>\($0.element.1)" }
    print("Original patch identical: \(current == baseline)")
    if !differences.isEmpty { print("Changed bytes: " + differences.joined(separator:", ")) }
    if current != baseline { exit(1) }
} else { fatalError("Unknown mode; nothing changed") }
