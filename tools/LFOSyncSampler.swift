// Hardware evidence collector: changes only the temporary edit buffer, never saves a preset.
// Usage: swift tools/LFOSyncSampler.swift NEW_OUTPUT_DIRECTORY [lfo-sync|modwheel-filter|osc-error|paraphonic|glide-divergence]
import Foundation
import CoreMIDI
import Darwin

struct CaptureError: Error, CustomStringConvertible {
    let description: String
    init(_ message: String) { description = message }
}
guard (2...3).contains(CommandLine.arguments.count) else {
    fatalError("Usage: swift tools/LFOSyncSampler.swift NEW_OUTPUT_DIRECTORY [lfo-sync|modwheel-filter|osc-error|paraphonic|glide-divergence]")
}
let group = CommandLine.arguments.count == 3 ? CommandLine.arguments[2] : "lfo-sync"
guard ["lfo-sync", "modwheel-filter", "osc-error", "paraphonic", "glide-divergence"].contains(group) else { fatalError("Unknown capture group") }
let directory = URL(fileURLWithPath: CommandLine.arguments[1], isDirectory: true)
guard !FileManager.default.fileExists(atPath: directory.path) else {
    fatalError("Refusing to overwrite an existing capture directory")
}
try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
var client = MIDIClientRef(), input = MIDIPortRef(), output = MIDIPortRef()
let lock = NSLock(), ready = DispatchSemaphore(value: 0)
var pending = [UInt8](), response: [UInt8]?
func valid(_ bytes: [UInt8]) -> Bool {
    bytes.count == 154 && Array(bytes.prefix(6)) == [0xF0,0,0x20,0x29,0,0x33]
        && bytes.last == 0xF7 && bytes.dropFirst().dropLast().allSatisfy { $0 < 128 }
}
guard MIDIClientCreateWithBlock("BSII LFO sync sampler" as CFString, &client, nil) == noErr else {
    fatalError("Cannot create MIDI client")
}
defer { MIDIPortDispose(input); MIDIPortDispose(output); MIDIClientDispose(client) }
guard MIDIInputPortCreateWithBlock(client, "LFO capture" as CFString, &input, { list, _ in
    let offset = MemoryLayout<MIDIPacketList>.offset(of: \.packet)!
    var packet = UnsafeRawPointer(list).advanced(by: offset).assumingMemoryBound(to: MIDIPacket.self)
    for _ in 0..<list.pointee.numPackets {
        let dataOffset = MemoryLayout<MIDIPacket>.offset(of: \.data)!
        let data = UnsafeRawPointer(packet).advanced(by: dataOffset).assumingMemoryBound(to: UInt8.self)
        lock.lock()
        for byte in UnsafeBufferPointer(start: data, count: Int(packet.pointee.length)) {
            if byte >= 0xF8 { continue }
            if byte == 0xF0 { pending = [byte] }
            else if !pending.isEmpty {
                pending.append(byte)
                if byte == 0xF7 {
                    if valid(pending) { response = pending; ready.signal() }
                    pending.removeAll()
                } else if pending.count > 4096 { pending.removeAll() }
            }
        }
        lock.unlock()
        packet = UnsafePointer(MIDIPacketNext(packet))
    }
}) == noErr else { fatalError("Cannot create input port") }
func name(_ endpoint: MIDIEndpointRef) -> String {
    var value: Unmanaged<CFString>?
    MIDIObjectGetStringProperty(endpoint, kMIDIPropertyDisplayName, &value)
    return value?.takeRetainedValue() as String? ?? ""
}
let sources = (0..<MIDIGetNumberOfSources()).map { MIDIGetSource($0) }.filter { name($0).lowercased().contains("bass station") }
let destinations = (0..<MIDIGetNumberOfDestinations()).map { MIDIGetDestination($0) }.filter { name($0).lowercased().contains("bass station") }
guard sources.count == 1 && destinations.count == 1 else { fatalError("Expected exactly one BSII input/output") }
guard MIDIPortConnectSource(input, sources[0], nil) == noErr,
      MIDIOutputPortCreate(client, "LFO test" as CFString, &output) == noErr else { fatalError("Cannot connect MIDI") }
func send(_ bytes: [UInt8]) throws {
    let storage = UnsafeMutableRawPointer.allocate(byteCount: 2048, alignment: 8)
    defer { storage.deallocate() }
    let list = storage.bindMemory(to: MIDIPacketList.self, capacity: 1)
    let first = MIDIPacketListInit(list)
    let added: UnsafeMutablePointer<MIDIPacket>? = bytes.withUnsafeBufferPointer { MIDIPacketListAdd(list, 2048, first, 0, $0.count, $0.baseAddress!) }
    guard added != nil && MIDISend(output, destinations[0], list) == noErr else { throw CaptureError("MIDI send failed") }
}
func patch() throws -> [UInt8] {
    while ready.wait(timeout: .now()) == .success {}
    lock.lock(); response = nil; lock.unlock()
    try send([0xF0,0,0x20,0x29,0,0x33,0,0x40,0xF7])
    guard ready.wait(timeout: .now() + 3) == .success else { throw CaptureError("Patch response timed out") }
    lock.lock(); defer { lock.unlock() }
    guard let bytes = response else { throw CaptureError("Missing response") }
    return bytes
}
func setNRPN(_ address: UInt8, _ value: Int) throws {
    let messages: [[UInt8]] = [[0xB0,99,0], [0xB0,98,address], [0xB0,6,UInt8(value)]]
    for bytes in messages {
        try send(bytes); Thread.sleep(forTimeInterval: 0.005)
    }
    Thread.sleep(forTimeInterval: 0.08)
}
struct Field {
    let id: String, address: UInt8, maximum: Int, masks: [Int: UInt8]
    let read: ([UInt8]) -> Int
}
// Candidate division masks inferred from labelled hardware endpoints.
let fields = group == "modwheel-filter" ? [
    Field(id: "modwheel-filter", address: 94, maximum: 127, masks: [82: 0x1F, 83: 0x60], read: { (Int($0[82] & 31) << 2) | Int(($0[83] & 96) >> 5) }),
    Field(id: "modwheel-lfo2-filter", address: 71, maximum: 127, masks: [84: 0x07, 85: 0x78], read: { (Int($0[84] & 7) << 4) | Int(($0[85] & 120) >> 3) })
] : group == "osc-error" ? [
    Field(id: "osc-error", address: 111, maximum: 7, masks: [117: 0x38], read: { Int(($0[117] & 0x38) >> 3) })
] : group == "paraphonic" ? [
    Field(id: "paraphonic", address: 107, maximum: 1, masks: [111: 0x02], read: { Int(($0[111] & 0x02) >> 1) })
] : group == "glide-divergence" ? [
    Field(id: "glide-divergence", address: 113, maximum: 15, masks: [118: 0x3C], read: { Int(($0[118] & 0x3C) >> 2) })
] : [
    Field(id: "lfo1-sync", address: 88, maximum: 1, masks: [69: 0x08], read: { Int(($0[69] & 8) >> 3) }),
    Field(id: "lfo2-sync", address: 92, maximum: 1, masks: [76: 0x10], read: { Int(($0[76] & 16) >> 4) }),
    Field(id: "lfo1-division", address: 87, maximum: 34, masks: [67: 0x1F, 68: 0x70], read: { (Int($0[67] & 31) << 3) | Int(($0[68] & 112) >> 4) }),
    Field(id: "lfo2-division", address: 91, maximum: 34, masks: [74: 0x3F, 75: 0x60], read: { (Int($0[74] & 63) << 2) | Int(($0[75] & 96) >> 5) })
]
var baseline: [UInt8]?
var problem: Error?
var rows = ["control,sent,decoded,unrelated_bytes_changed"]
do {
    let original = try patch()
    try Data(original).write(to: directory.appendingPathComponent("before.syx"), options: .withoutOverwriting)
    baseline = original
    var previous = original
    for field in fields {
        for value in 0...field.maximum {
            try setNRPN(field.address, value)
            let bytes = try patch()
            try Data(bytes).write(to: directory.appendingPathComponent("\(field.id)-\(value).syx"), options: .withoutOverwriting)
            let unexpected = bytes.indices.filter { ((bytes[$0] ^ previous[$0]) & ~(field.masks[$0] ?? 0)) != 0 }
            rows.append("\(field.id),\(value),\(field.read(bytes)),\(unexpected.count)")
            guard field.read(bytes) == value && unexpected.isEmpty else {
                throw CaptureError("Mismatch at \(field.id)=\(value); unrelated changed bytes: \(unexpected)")
            }
            previous = bytes
        }
        print("PASS \(field.id): \(field.maximum + 1) values, no unrelated bits changed"); fflush(stdout)
    }
} catch { problem = error }
// Always attempt restoration after any caught send/capture/file error once a backup exists.
var restored = false
if let original = baseline {
    do {
        try send(original); Thread.sleep(forTimeInterval: 0.3)
        let after = try patch()
        try Data(after).write(to: directory.appendingPathComponent("after.syx"), options: .withoutOverwriting)
        restored = after == original
        if !restored { print("RESTORE MISMATCH: backup retained at \(directory.path)/before.syx") }
    } catch { print("RESTORE FAILED: \(error); backup retained at \(directory.path)/before.syx") }
}
try (rows.joined(separator: "\n") + "\n").write(to: directory.appendingPathComponent("samples.csv"), atomically: true, encoding: .utf8)
print("Full starting patch restored and verified: \(restored)")
if let error = problem { print("FAILED: \(error)") }
if problem != nil || !restored { exit(1) }
