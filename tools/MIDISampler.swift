import Foundation
import CoreMIDI

guard (2...3).contains(CommandLine.arguments.count) else { fatalError("Usage: swift tools/MIDISampler.swift OUTPUT_DIRECTORY [osc2-coarse|mix-osc1|mix-osc2|mix-sub|mix-noise|mix-ring|mix-external|filter-cutoff|filter-resonance|filter-overdrive|filter-modenv|filter-lfo2|filter-tracking|amp-attack|amp-decay|amp-sustain|amp-release]. Use routing-fixed BSII Control or close older app versions.") }
let outputDirectory = URL(fileURLWithPath: CommandLine.arguments[1], isDirectory: true)
try FileManager.default.createDirectory(at: outputDirectory, withIntermediateDirectories: true)
let control = CommandLine.arguments.count == 3 ? CommandLine.arguments[2] : "osc2-coarse"
guard ["osc2-coarse", "mix-osc1", "mix-osc2", "mix-sub", "mix-noise", "mix-ring", "mix-external", "filter-cutoff", "filter-resonance", "filter-overdrive", "filter-modenv", "filter-lfo2", "filter-tracking", "amp-attack", "amp-decay", "amp-sustain", "amp-release"].contains(control) else { fatalError("Unknown control") }
let msbCC: UInt8 = control == "mix-osc1" ? 20 : control == "mix-osc2" ? 21 : control == "mix-sub" ? 22 : control == "mix-noise" ? 23 : control == "mix-ring" ? 24 : control == "mix-external" ? 25 : control == "filter-cutoff" ? 16 : control == "filter-resonance" ? 82 : control == "filter-overdrive" ? 114 : control == "filter-modenv" ? 85 : control == "filter-lfo2" ? 17 : control == "amp-attack" ? 90 : control == "amp-decay" ? 91 : control == "amp-sustain" ? 92 : control == "amp-release" ? 93 : 30
let lsbCC: UInt8 = control == "mix-osc1" ? 52 : control == "mix-osc2" ? 53 : control == "mix-sub" ? 54 : control == "mix-noise" ? 55 : control == "mix-ring" ? 56 : control == "mix-external" ? 57 : control == "filter-cutoff" ? 48 : control == "filter-lfo2" ? 49 : 62
let singleCC = ["filter-resonance", "filter-overdrive", "filter-modenv", "amp-attack", "amp-decay", "amp-sustain", "amp-release"].contains(control)
let nrpn = control == "filter-tracking"
let maximumRaw = nrpn ? 7 : singleCC ? 127 : 255
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
    if control == "mix-ring" { return ((Int(b[42]) & 63) << 2) | ((Int(b[43]) & 96) >> 5) }
    if control == "mix-external" { return ((Int(b[43]) & 31) << 3) | ((Int(b[44]) & 112) >> 4) }
    if control == "filter-cutoff" { return ((Int(b[44]) & 15) << 4) | ((Int(b[45]) >> 3) & 15) }
    if control == "filter-resonance" { return ((Int(b[45]) & 3) << 5) | ((Int(b[46]) >> 2) & 31) }
    if control == "filter-overdrive" { return ((Int(b[46]) & 1) << 6) | ((Int(b[47]) >> 1) & 63) }
    if control == "filter-modenv" { return ((Int(b[105]) & 63) << 1) | ((Int(b[106]) & 64) >> 6) }
    if control == "filter-lfo2" { return ((Int(b[97]) & 127) << 1) | ((Int(b[98]) & 64) >> 6) }
    if control == "filter-tracking" { return Int(b[112]) & 7 }
    if control == "amp-attack" { return ((Int(b[50]) & 31) << 2) | ((Int(b[51]) >> 5) & 3) }
    if control == "amp-decay" { return ((Int(b[51]) & 15) << 3) | ((Int(b[52]) & 112) >> 4) }
    if control == "amp-sustain" { return ((Int(b[52]) & 7) << 4) | ((Int(b[53]) & 120) >> 3) }
    if control == "amp-release" { return ((Int(b[53]) & 3) << 5) | ((Int(b[54]) & 124) >> 2) }
    return ((Int(b[27]) & 31) << 3) | ((Int(b[28]) & 112) >> 4)
}
func setValue(_ raw: Int) {
    if nrpn {
        send([0xB0,99,0]); Thread.sleep(forTimeInterval: 0.005)
        send([0xB0,98,108]); Thread.sleep(forTimeInterval: 0.005)
        send([0xB0,6,UInt8(raw)]); Thread.sleep(forTimeInterval: 0.08)
        return
    }
    if singleCC {
        send([0xB0,msbCC,UInt8(raw)])
        Thread.sleep(forTimeInterval: 0.08)
        return
    }
    send([0xB0,msbCC,UInt8(raw >> 1)])
    Thread.sleep(forTimeInterval: 0.005)
    send([0xB0,lsbCC,UInt8((raw & 1) << 6)])
    Thread.sleep(forTimeInterval: 0.08)
}
guard let baseline = patch() else { fatalError("No baseline patch response; no parameters changed") }
try Data(baseline).write(to: outputDirectory.appendingPathComponent("before.syx"))
let original = readValue(baseline)
print("Baseline \(control) raw: \(original). Sampling \(maximumRaw + 1) values."); fflush(stdout)
var rows = ["sent_raw,msb_value,lsb_value,sysex_raw,match"]
var failures = 0
for raw in 0...maximumRaw {
    setValue(raw)
    guard let result = patch() else { print("Timeout at \(raw); stopping sweep"); failures += 1; break }
    let actual = readValue(result)
    if actual != raw { failures += 1 }
    rows.append("\(raw),\((singleCC || nrpn) ? raw : raw >> 1),\((singleCC || nrpn) ? "" : String((raw & 1) << 6)),\(actual),\(actual == raw)")
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
exit(failures == 0 && restoreOK && rows.count == maximumRaw + 2 ? 0 : 1)
