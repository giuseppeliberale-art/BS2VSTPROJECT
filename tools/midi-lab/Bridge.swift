// CoreMIDI transport only. Decode/analyse bytes in lab.py.
// listen creates no output port. request writes a backup; restore is explicit.
import Foundation
import CoreMIDI
import Darwin

func fail(_ text: String) -> Never { fputs(text + "\n", stderr); exit(1) }
let args = Array(CommandLine.arguments.dropFirst())
guard args.count >= 2, ["listen", "request", "restore"].contains(args[0]) else {
    fail("Usage: Bridge listen SECONDS | request OUTPUT.syx | restore INPUT.syx")
}
let mode = args[0]
var client = MIDIClientRef(), input = MIDIPortRef(), output = MIDIPortRef()
let lock = NSLock()
var sysex = [UInt8](), response: [UInt8]?
let ready = DispatchSemaphore(value: 0)
func name(_ endpoint: MIDIEndpointRef) -> String {
    var text: Unmanaged<CFString>?
    MIDIObjectGetStringProperty(endpoint, kMIDIPropertyDisplayName, &text)
    return text?.takeRetainedValue() as String? ?? ""
}
func matching(_ endpoints: [MIDIEndpointRef]) -> [MIDIEndpointRef] {
    endpoints.filter { name($0).lowercased().contains("bass station") }
}
guard MIDIClientCreateWithBlock("BSII MIDI Lab" as CFString, &client, nil) == noErr else { fail("Cannot create MIDI client") }
defer { if input != 0 { MIDIPortDispose(input) }; if output != 0 { MIDIPortDispose(output) }; MIDIClientDispose(client) }
let start = Date()
guard MIDIInputPortCreateWithBlock(client, "lab input" as CFString, &input, { list, _ in
    let offset = MemoryLayout<MIDIPacketList>.offset(of: \.packet)!
    var packet = UnsafeRawPointer(list).advanced(by: offset).assumingMemoryBound(to: MIDIPacket.self)
    for _ in 0..<list.pointee.numPackets {
        // The data may extend past the fixed imported Swift tuple for long packets.
        let dataOffset = MemoryLayout<MIDIPacket>.offset(of: \.data)!
        let raw = UnsafeRawPointer(packet).advanced(by: dataOffset).assumingMemoryBound(to: UInt8.self)
        let bytes = Array(UnsafeBufferPointer(start: raw, count: Int(packet.pointee.length)))
        lock.lock()
        if mode == "listen" {
            let row: [String: Any] = ["seconds": Date().timeIntervalSince(start), "bytes": bytes]
            if let data = try? JSONSerialization.data(withJSONObject: row, options: [.sortedKeys]) {
                print(String(decoding: data, as: UTF8.self)); fflush(stdout)
            }
        } else {
            for byte in bytes {
                if byte >= 0xF8 { continue }
                if byte == 0xF0 { sysex = [byte] }
                else if !sysex.isEmpty {
                    sysex.append(byte)
                    if byte == 0xF7 {
                        if sysex.count == 154 && Array(sysex.prefix(6)) == [0xF0,0,0x20,0x29,0,0x33]
                            && sysex.dropFirst().dropLast().allSatisfy({ $0 < 128 }) {
                            response = sysex; ready.signal()
                        }
                        sysex.removeAll()
                    } else if sysex.count > 4096 { sysex.removeAll() }
                }
            }
        }
        lock.unlock()
        packet = UnsafePointer(MIDIPacketNext(packet))
    }
}) == noErr else { fail("Cannot create MIDI input") }
let sources = matching((0..<MIDIGetNumberOfSources()).map { MIDIGetSource($0) })
guard sources.count == 1 else { fail("Expected exactly one Bass Station II input, found \(sources.count)") }
guard MIDIPortConnectSource(input, sources[0], nil) == noErr else { fail("Cannot connect input") }
fputs("Connected: \(name(sources[0]))\n", stderr)
if mode == "listen" {
    guard let seconds = Double(args[1]), seconds > 0, seconds <= 3600 else { fail("Seconds must be 1…3600") }
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN)
    var stopping = false
    let interrupt = DispatchSource.makeSignalSource(signal: SIGINT, queue: .main)
    let terminate = DispatchSource.makeSignalSource(signal: SIGTERM, queue: .main)
    interrupt.setEventHandler { stopping = true }; interrupt.resume()
    terminate.setEventHandler { stopping = true }; terminate.resume()
    while !stopping && Date().timeIntervalSince(start) < seconds {
        RunLoop.current.run(until: Date().addingTimeInterval(0.05))
    }
    interrupt.cancel(); terminate.cancel()
    exit(0)
}
let destinations = matching((0..<MIDIGetNumberOfDestinations()).map { MIDIGetDestination($0) })
guard destinations.count == 1 else { fail("Expected exactly one Bass Station II output") }
guard MIDIOutputPortCreate(client, "lab output" as CFString, &output) == noErr else { fail("Cannot create output") }
func send(_ bytes: [UInt8]) {
    let size = max(2048, bytes.count + 64)
    let storage = UnsafeMutableRawPointer.allocate(byteCount: size, alignment: 8)
    defer { storage.deallocate() }
    let list = storage.bindMemory(to: MIDIPacketList.self, capacity: 1)
    let first = MIDIPacketListInit(list)
    let added = bytes.withUnsafeBufferPointer { MIDIPacketListAdd(list, size, first, 0, bytes.count, $0.baseAddress!) }
    guard added != nil && MIDISend(output, destinations[0], list) == noErr else { fail("MIDI send failed") }
}
let path = URL(fileURLWithPath: args[1])
if mode == "restore" {
    let bytes = Array(try Data(contentsOf: path))
    guard bytes.count == 154, Array(bytes.prefix(6)) == [0xF0,0,0x20,0x29,0,0x33], bytes.last == 0xF7,
          bytes.dropFirst().dropLast().allSatisfy({ $0 < 128 }) else { fail("Not a single valid-framed BSII patch") }
    send(bytes); Thread.sleep(forTimeInterval: 0.3)
} else if FileManager.default.fileExists(atPath: path.path) { fail("Refusing to overwrite backup") }
lock.lock(); response = nil; lock.unlock()
while ready.wait(timeout: .now()) == .success {}
send([0xF0,0,0x20,0x29,0,0x33,0,0x40,0xF7])
guard ready.wait(timeout: .now() + 3) == .success else { fail("No patch response; backup not written / restore unverified") }
lock.lock(); let captured = response; lock.unlock()
guard let captured = captured else { fail("No response") }
if mode == "restore" {
    guard Data(captured) == (try Data(contentsOf: path)) else { fail("RESTORE FAILED: full patch readback differs; backup retained") }
    print("Full patch restoration verified")
} else {
    try Data(captured).write(to: path, options: .withoutOverwriting)
    print("Saved 154-byte patch")
}
