import Foundation
import CoreMIDI

guard (2...3).contains(CommandLine.arguments.count) else { fatalError("Usage: swift tools/MIDISampler.swift OUTPUT_DIRECTORY [osc2-coarse|mix-osc1|mix-osc2|mix-sub|mix-noise|mix-ring|mix-external|filter-cutoff|filter-resonance|filter-overdrive|filter-modenv|filter-lfo2|filter-tracking|amp-attack|amp-decay|amp-sustain|amp-release|mod-attack|mod-decay|mod-sustain|mod-release|lfo1-speed|lfo2-speed|lfo1-wave|lfo2-wave|lfo1-delay|lfo2-delay|lfo1-slew|lfo2-slew|lfo1-keysync|lfo2-keysync|distortion|osc-filter-mod|vca-limit|modwheel-lfo1-pitch|modwheel-osc2-pitch|aftertouch-filter|aftertouch-lfo1|aftertouch-lfo2-speed|portamento|bend-amount|arp-swing|velocity-amp|velocity-mod|osc-sync|osc1-modenv-pitch|osc2-modenv-pitch|osc1-lfo1-pitch|osc2-lfo1-pitch|osc1-modenv-pw|osc2-modenv-pw|osc1-lfo2-pw|osc2-lfo2-pw|osc1-manual-pw|osc2-manual-pw|patch-volume|sub-octave|sub-wave|osc1-wave|osc2-wave]. Use routing-fixed BSII Control or close older app versions.") }
let outputDirectory = URL(fileURLWithPath: CommandLine.arguments[1], isDirectory: true)
try FileManager.default.createDirectory(at: outputDirectory, withIntermediateDirectories: true)
let control = CommandLine.arguments.count == 3 ? CommandLine.arguments[2] : "osc2-coarse"
guard ["osc2-coarse", "mix-osc1", "mix-osc2", "mix-sub", "mix-noise", "mix-ring", "mix-external", "filter-cutoff", "filter-resonance", "filter-overdrive", "filter-modenv", "filter-lfo2", "filter-tracking", "amp-attack", "amp-decay", "amp-sustain", "amp-release", "mod-attack", "mod-decay", "mod-sustain", "mod-release", "lfo1-speed", "lfo2-speed", "lfo1-wave", "lfo2-wave", "lfo1-delay", "lfo2-delay", "lfo1-slew", "lfo2-slew", "lfo1-keysync", "lfo2-keysync", "distortion", "osc-filter-mod", "vca-limit", "modwheel-lfo1-pitch", "modwheel-osc2-pitch", "aftertouch-filter", "aftertouch-lfo1", "aftertouch-lfo2-speed", "portamento", "bend-amount", "arp-swing", "velocity-amp", "velocity-mod", "osc-sync", "osc1-modenv-pitch", "osc2-modenv-pitch", "osc1-lfo1-pitch", "osc2-lfo1-pitch", "osc1-modenv-pw", "osc2-modenv-pw", "osc1-lfo2-pw", "osc2-lfo2-pw", "osc1-manual-pw", "osc2-manual-pw", "patch-volume", "sub-octave", "sub-wave", "osc1-wave", "osc2-wave"].contains(control) else { fatalError("Unknown control") }
let oscillatorCC: [String: UInt8] = ["patch-volume": 7, "sub-octave": 81, "sub-wave": 80, "osc1-modenv-pitch": 71, "osc2-modenv-pitch": 76, "osc1-lfo1-pitch": 28, "osc2-lfo1-pitch": 31, "osc1-modenv-pw": 72, "osc2-modenv-pw": 77, "osc1-lfo2-pw": 73, "osc2-lfo2-pw": 78, "osc1-manual-pw": 74, "osc2-manual-pw": 79]
let msbCC: UInt8 = oscillatorCC[control] ?? (control == "mix-osc1" ? 20 : control == "mix-osc2" ? 21 : control == "mix-sub" ? 22 : control == "mix-noise" ? 23 : control == "mix-ring" ? 24 : control == "mix-external" ? 25 : control == "filter-cutoff" ? 16 : control == "filter-resonance" ? 82 : control == "filter-overdrive" ? 114 : control == "filter-modenv" ? 85 : control == "filter-lfo2" ? 17 : control == "amp-attack" ? 90 : control == "amp-decay" ? 91 : control == "amp-sustain" ? 92 : control == "amp-release" ? 93 : control == "mod-attack" ? 102 : control == "mod-decay" ? 103 : control == "mod-sustain" ? 104 : control == "mod-release" ? 105 : control == "lfo1-speed" ? 18 : control == "lfo2-speed" ? 19 : control == "lfo1-wave" ? 88 : control == "lfo2-wave" ? 89 : control == "lfo1-delay" ? 86 : control == "lfo2-delay" ? 87 : control == "distortion" ? 94 : control == "osc-filter-mod" ? 115 : control == "vca-limit" ? 95 : control == "portamento" ? 5 : control == "bend-amount" ? 107 : control == "arp-swing" ? 116 : control == "velocity-amp" ? 112 : control == "velocity-mod" ? 113 : control == "osc-sync" ? 110 : 30)
let lsbCC: UInt8 = control == "osc1-lfo1-pitch" ? 60 : control == "osc2-lfo1-pitch" ? 63 : control == "mix-osc1" ? 52 : control == "mix-osc2" ? 53 : control == "mix-sub" ? 54 : control == "mix-noise" ? 55 : control == "mix-ring" ? 56 : control == "mix-external" ? 57 : control == "filter-cutoff" ? 48 : control == "filter-lfo2" ? 49 : control == "lfo1-speed" ? 50 : control == "lfo2-speed" ? 51 : 62
let singleCC = ["filter-resonance", "filter-overdrive", "filter-modenv", "amp-attack", "amp-decay", "amp-sustain", "amp-release", "mod-attack", "mod-decay", "mod-sustain", "mod-release", "lfo1-wave", "lfo2-wave", "lfo1-delay", "lfo2-delay", "distortion", "osc-filter-mod", "vca-limit", "portamento", "bend-amount", "arp-swing", "velocity-amp", "velocity-mod", "osc-sync", "osc1-modenv-pitch", "osc2-modenv-pitch", "osc1-modenv-pw", "osc2-modenv-pw", "osc1-lfo2-pw", "osc2-lfo2-pw", "osc1-manual-pw", "osc2-manual-pw", "patch-volume", "sub-octave", "sub-wave"].contains(control)
let nrpnNumber: UInt8? = control == "filter-tracking" ? 108 : control == "lfo1-slew" ? 86 : control == "lfo2-slew" ? 90 : control == "lfo1-keysync" ? 89 : control == "lfo2-keysync" ? 93 : control == "modwheel-lfo1-pitch" ? 70 : control == "modwheel-osc2-pitch" ? 78 : control == "aftertouch-filter" ? 74 : control == "aftertouch-lfo1" ? 75 : control == "aftertouch-lfo2-speed" ? 76 : control == "osc1-wave" ? 72 : control == "osc2-wave" ? 82 : nil
let nrpn = nrpnNumber != nil
let minimumRaw = control == "bend-amount" ? 1 : 0
let maximumRaw = control == "sub-wave" ? 2 : control == "bend-amount" ? 12 : control.hasSuffix("-keysync") ? 1 : control.hasSuffix("-wave") ? 3 : control == "filter-tracking" ? 7 : (singleCC || nrpn) ? 127 : 255
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
    if control == "mod-attack" { return ((Int(b[57]) & 63) << 1) | ((Int(b[58]) & 64) >> 6) }
    if control == "mod-decay" { return ((Int(b[58]) & 31) << 2) | ((Int(b[59]) & 96) >> 5) }
    if control == "mod-sustain" { return ((Int(b[59]) & 15) << 3) | ((Int(b[60]) & 112) >> 4) }
    if control == "mod-release" { return ((Int(b[60]) & 7) << 4) | ((Int(b[61]) & 120) >> 3) }
    if control == "lfo1-speed" { return ((Int(b[66]) & 63) << 2) | ((Int(b[67]) & 96) >> 5) }
    if control == "lfo2-speed" { return ((Int(b[73]) & 127) << 1) | ((Int(b[74]) & 64) >> 6) }
    if control == "lfo1-wave" { return (Int(b[63]) & 6) >> 1 }
    if control == "lfo2-wave" { return (Int(b[70]) & 12) >> 2 }
    if control == "lfo1-delay" { return Int(b[64]) & 127 }
    if control == "lfo2-delay" { return ((Int(b[70]) & 1) << 6) | ((Int(b[71]) & 126) >> 1) }
    if control == "lfo1-slew" { return ((Int(b[65]) & 63) << 1) | ((Int(b[66]) & 64) >> 6) }
    if control == "lfo2-slew" { return Int(b[72]) & 127 }
    if control == "lfo1-keysync" { return (Int(b[69]) & 16) >> 4 }
    if control == "lfo2-keysync" { return (Int(b[76]) & 32) >> 5 }
    if control == "osc-filter-mod" { return ((Int(b[106]) & 31) << 2) | ((Int(b[107]) & 96) >> 5) }
    if control == "distortion" { return ((Int(b[107]) & 15) << 3) | ((Int(b[108]) & 112) >> 4) }
    if control == "vca-limit" { return ((Int(b[108]) & 7) << 4) | ((Int(b[109]) & 120) >> 3) }
    if control == "modwheel-lfo1-pitch" { return ((Int(b[83]) & 15) << 3) | ((Int(b[84]) & 112) >> 4) }
    if control == "modwheel-osc2-pitch" { return ((Int(b[85]) & 3) << 5) | ((Int(b[86]) & 124) >> 2) }
    if control == "aftertouch-filter" { return ((Int(b[86]) & 1) << 6) | ((Int(b[87]) & 126) >> 1) }
    if control == "aftertouch-lfo1" { return Int(b[88]) & 127 }
    if control == "aftertouch-lfo2-speed" { return ((Int(b[89]) & 63) << 1) | ((Int(b[90]) & 64) >> 6) }
    if control == "portamento" { return ((Int(b[13]) & 3) << 5) | ((Int(b[14]) & 124) >> 2) }
    if control == "bend-amount" { return (Int(b[16]) & 127) - 64 }
    if control == "arp-swing" { return ((Int(b[81]) & 63) << 1) | ((Int(b[82]) & 64) >> 6) }
    if control == "velocity-amp" { return ((Int(b[49]) & 63) << 1) | ((Int(b[50]) & 64) >> 6) }
    if control == "velocity-mod" { return Int(b[56]) & 127 }
    if control == "osc-sync" { return (b[18] & 64) == 0 ? 0 : 127 }
    if control == "osc1-modenv-pitch" { return ((Int(b[98]) & 31) << 2) | ((Int(b[99]) & 96) >> 5) }
    if control == "osc2-modenv-pitch" { return ((Int(b[99]) & 15) << 3) | ((Int(b[100]) & 112) >> 4) }
    if control == "osc1-lfo1-pitch" { return ((Int(b[90]) & 63) << 2) | ((Int(b[91]) & 96) >> 5) }
    if control == "osc2-lfo1-pitch" { return ((Int(b[91]) & 31) << 3) | ((Int(b[92]) & 112) >> 4) }
    if control == "osc1-modenv-pw" { return ((Int(b[101]) & 3) << 5) | ((Int(b[102]) & 124) >> 2) }
    if control == "osc2-modenv-pw" { return ((Int(b[102]) & 1) << 6) | ((Int(b[103]) & 126) >> 1) }
    if control == "osc1-lfo2-pw" { return ((Int(b[93]) & 3) << 5) | ((Int(b[94]) & 124) >> 2) }
    if control == "osc2-lfo2-pw" { return ((Int(b[94]) & 1) << 6) | ((Int(b[95]) & 126) >> 1) }
    if control == "osc1-manual-pw" { return ((Int(b[19]) & 15) << 3) | ((Int(b[20]) & 112) >> 4) }
    if control == "osc2-manual-pw" { return ((Int(b[25]) & 63) << 1) | ((Int(b[26]) & 64) >> 6) }
    if control == "patch-volume" { return ((Int(b[109]) & 3) << 5) | ((Int(b[110]) & 124) >> 2) }
    if control == "sub-octave" { return (b[37] & 8) == 0 ? 0 : 127 }
    if control == "sub-wave" { return (Int(b[36]) & 48) >> 4 }
    if control == "osc1-wave" { return (Int(b[19]) & 96) >> 5 }
    if control == "osc2-wave" { return Int(b[24]) & 3 }
    return ((Int(b[27]) & 31) << 3) | ((Int(b[28]) & 112) >> 4)
}
func setValue(_ raw: Int) {
    if nrpn {
        send([0xB0,99,0]); Thread.sleep(forTimeInterval: 0.005)
        send([0xB0,98,nrpnNumber!]); Thread.sleep(forTimeInterval: 0.005)
        send([0xB0,6,UInt8(raw)]); Thread.sleep(forTimeInterval: 0.08)
        return
    }
    if singleCC {
        send([0xB0,msbCC,UInt8(control == "bend-amount" ? raw + 64 : raw)])
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
print("Baseline \(control) raw: \(original). Sampling \(maximumRaw - minimumRaw + 1) values."); fflush(stdout)
var rows = ["sent_raw,msb_value,lsb_value,sysex_raw,match"]
var failures = 0
for raw in minimumRaw...maximumRaw {
    setValue(raw)
    guard let result = patch() else { print("Timeout at \(raw); stopping sweep"); failures += 1; break }
    let actual = readValue(result)
    let expected = control == "arp-swing" ? min(97, max(3, raw)) : control.hasPrefix("velocity-") ? max(1, raw) : control == "osc-sync" ? (raw == 0 ? 0 : 127) : control == "sub-octave" ? (raw < 63 ? 0 : 127) : raw
    if actual != expected { failures += 1 }
    rows.append("\(raw),\(control == "bend-amount" ? raw + 64 : (singleCC || nrpn) ? raw : raw >> 1),\((singleCC || nrpn) ? "" : String((raw & 1) << 6)),\(actual),\(actual == expected)")
    try Data(result).write(to: outputDirectory.appendingPathComponent(String(format: "raw-%03d.syx", raw)))
    if raw % 32 == 0 { print("Sample \(raw): received \(actual)"); fflush(stdout) }
}
send(baseline)
Thread.sleep(forTimeInterval: 0.3)
let restored = patch()
let restoreOK = restored.map { $0 == baseline } ?? false
if let restored = restored { try Data(restored).write(to: outputDirectory.appendingPathComponent("after.syx")) }
try (rows.joined(separator:"\n")+"\n").write(to: outputDirectory.appendingPathComponent("\(control).csv"), atomically: true, encoding: .utf8)
let summary = "Samples: \(rows.count-1)\nMismatches/timeouts: \(failures)\nOriginal raw value: \(original)\nEntire starting patch restored and verified: \(restoreOK)\n"
try summary.write(to: outputDirectory.appendingPathComponent("summary.txt"), atomically:true, encoding:.utf8)
print(summary)
MIDIPortDispose(input); MIDIPortDispose(output); MIDIClientDispose(client)
exit(failures == 0 && restoreOK && rows.count == maximumRaw - minimumRaw + 2 ? 0 : 1)
