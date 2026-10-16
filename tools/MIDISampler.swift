import Foundation
import CoreMIDI

guard (2...3).contains(CommandLine.arguments.count) else { fatalError("Usage: swift tools/MIDISampler.swift OUTPUT_DIRECTORY [osc2-coarse|mix-osc1|mix-osc2|mix-sub|mix-noise]. Use routing-fixed BSII Control or close older app versions.") }
let outputDirectory = URL(fileURLWithPath: CommandLine.arguments[1], isDirectory: true)
try FileManager.default.createDirectory(at: outputDirectory, withIntermediateDirectories: true)
let control = CommandLine.arguments.count == 3 ? CommandLine.arguments[2] : "osc2-coarse"
guard ["osc2-coarse", "mix-osc1", "mix-osc2", "mix-sub", "mix-noise"].contains(control) else { fatalError("Unknown control") }
let msbCC: UInt8 = control == "mix-osc1" ? 20 : control == "mix-osc2" ? 21 : control == "mix-sub" ? 22 : control == "mix-noise" ? 23 : 30
let lsbCC: UInt8 = control == "mix-osc1" ? 52 : control == "mix-osc2" ? 53 : control == "mix-sub" ? 54 : control == "mix-noise" ? 55 : 62
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
func readValue(_ b: [UInt8]) -> Int {
    if control == "mix-osc1" { return ((Int(b[37]) & 7) << 5) | ((Int(b[38]) & 124) >> 2) }
    if control == "mix-osc2" { return ((Int(b[38]) & 3) << 6) | ((Int(b[39]) & 126) >> 1) }
    if control == "mix-sub" { return ((Int(b[39]) & 1) << 7) | (Int(b[40]) & 127) }
    if control == "mix-noise" { return ((Int(b[41]) & 127) << 1) | ((Int(b[42]) & 64) >> 6) }
    return ((Int(b[27]) & 31) << 3) | ((Int(b[28]) & 112) >> 4)
}
func setValue(_ raw: Int) {
    send([0xB0,msbCC,UInt8(raw >> 1)])
    Thread.sleep(forTimeInterval: 0.005)
    send([0xB0,lsbCC,UInt8((raw & 1) << 6)])
    Thread.sleep(forTimeInterval: 0.08)
}
guard let baseline = patch() else { fatalError("No baseline patch response; no parameters changed") }
try Data(baseline).write(to: outputDirectory.appendingPathComponent("before.syx"))
let original = readValue(baseline)
print("Baseline \(control) raw: \(original). Sampling 256 values."); fflush(stdout)
var rows = ["sent_raw,msb_value,lsb_value,sysex_raw,match"]
var failures = 0
for raw in 0...255 {
    setValue(raw)
    guard let result = patch() else { print("Timeout at \(raw); stopping sweep"); failures += 1; break }
    let actual = readValue(result)
    if actual != raw { failures += 1 }
    rows.append("\(raw),\(raw >> 1),\((raw & 1) << 6),\(actual),\(actual == raw)")
    try Data(result).write(to: outputDirectory.appendingPathComponent(String(format: "raw-%03d.syx", raw)))
    if raw % 32 == 0 { print("Sample \(raw): received \(actual)"); fflush(stdout) }
}
setValue(original)
let restored = patch()
let restoreOK = restored.map { $0 == baseline } ?? false
if let restored = restored { try Data(restored).write(to: outputDirectory.appendingPathComponent("after.syx")) }
try (rows.joined(separator:"\n")+"\n").write(to: outputDirectory.appendingPathComponent("\(control).csv"), atomically: true, encoding: .utf8)
let summary = "Samples: \(rows.count-1)\nMismatches/timeouts: \(failures)\nOriginal raw value: \(original)\nEntire starting patch restored and verified: \(restoreOK)\n"
try summary.write(to: outputDirectory.appendingPathComponent("summary.txt"), atomically:true, encoding:.utf8)
print(summary)
MIDIPortDispose(input); MIDIPortDispose(output); MIDIClientDispose(client)
exit(failures == 0 && restoreOK && rows.count == 257 ? 0 : 1)
