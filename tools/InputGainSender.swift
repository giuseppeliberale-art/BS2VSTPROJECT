// Explicit, narrowly scoped global-setting probe; no preset-save or patch messages.
// Usage: swift tools/InputGainSender.swift RAW_VALUE (0...31)
import Foundation
import CoreMIDI
import Darwin

func fail(_ message: String) -> Never { fputs(message + "\n", stderr); exit(1) }
guard CommandLine.arguments.count == 2, let raw = Int(CommandLine.arguments[1]),
      (0...31).contains(raw) else { fail("Usage: swift tools/InputGainSender.swift RAW_VALUE (0...31)") }
func name(_ endpoint: MIDIEndpointRef) -> String {
    var text: Unmanaged<CFString>?
    MIDIObjectGetStringProperty(endpoint, kMIDIPropertyDisplayName, &text)
    return text?.takeRetainedValue() as String? ?? ""
}
let targets = (0..<MIDIGetNumberOfDestinations()).map { MIDIGetDestination($0) }
    .filter { name($0).lowercased().contains("bass station") }
guard targets.count == 1 else { fail("Expected exactly one Bass Station II destination") }
var client = MIDIClientRef(), port = MIDIPortRef()
guard MIDIClientCreateWithBlock("BSII Input Gain probe" as CFString, &client, nil) == noErr else { fail("Cannot create MIDI client") }
defer { if port != 0 { MIDIPortDispose(port) }; MIDIClientDispose(client) }
guard MIDIOutputPortCreate(client, "Input Gain output" as CFString, &port) == noErr else { fail("Cannot open output") }
let messages: [[UInt8]] = [[0xB0,99,64], [0xB0,98,28], [0xB0,6,UInt8(raw)]]
for bytes in messages {
    let storage = UnsafeMutableRawPointer.allocate(byteCount: 1024, alignment: 8)
    let list = storage.bindMemory(to: MIDIPacketList.self, capacity: 1)
    let first = MIDIPacketListInit(list)
    let added: UnsafeMutablePointer<MIDIPacket>? = bytes.withUnsafeBufferPointer {
        MIDIPacketListAdd(list, 1024, first, 0, $0.count, $0.baseAddress!)
    }
    let status = added == nil ? OSStatus(-1) : MIDISend(port, targets[0], list)
    storage.deallocate()
    guard status == noErr else { fail("MIDI send failed; hardware state must be checked manually") }
    Thread.sleep(forTimeInterval: 0.01)
}
Thread.sleep(forTimeInterval: 0.1)
print("Sent NRPN 64:28 raw \(raw) on channel 1. Hardware receipt is not verified by this output.")
