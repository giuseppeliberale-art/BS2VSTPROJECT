#include "BSIIPairedCC.h"
#include "BSIIMidiRouting.h"
#include "BSIIProjectRecall.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

BSIIControlAudioProcessor::BSIIControlAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createParameterLayout()),
      directSysExInput ([this] (const juce::MidiMessage& message)
      {
          if (message.isSysEx()) receiveSysEx (message);
          else
          {
              juce::MidiBuffer controls;
              controls.addEvent (message, 0);
              parseIncomingMidi (controls, false, true);
          }
      }, true)
{
    clearPendingMidi();
    for (auto& value : pendingHardwareValues) value.store (bsii::noHardwareValue, std::memory_order_relaxed);
    for (auto& value : pendingChoiceWireValues) value.store (-1, std::memory_order_relaxed);
    for (auto& value : pairedMSB) value.store (-1, std::memory_order_relaxed);
    for (auto& value : currentNrpnMSB) value.store (-1, std::memory_order_relaxed);
    for (auto& value : currentNrpnLSB) value.store (-1, std::memory_order_relaxed);
    for (const auto& spec : bsii::parameterSpecs()) apvts.addParameterListener (spec.id, this);
    for (const auto& spec : bsii::choiceSpecs()) apvts.addParameterListener (spec.id, this);
    startTimerHz (100);
}

BSIIControlAudioProcessor::~BSIIControlAudioProcessor()
{
    stopTimer();
    for (const auto& spec : bsii::parameterSpecs()) apvts.removeParameterListener (spec.id, this);
    for (const auto& spec : bsii::choiceSpecs()) apvts.removeParameterListener (spec.id, this);
}

thread_local BSIIControlAudioProcessor::ScopedMidiSilence* BSIIControlAudioProcessor::midiSilence = nullptr;

BSIIControlAudioProcessor::ScopedMidiSilence::ScopedMidiSilence (BSIIControlAudioProcessor& p, bool discard)
    : processor (p), previous (midiSilence), discardPendingTrigger (discard) { midiSilence = this; }
BSIIControlAudioProcessor::ScopedMidiSilence::~ScopedMidiSilence() { midiSilence = previous; }

bool BSIIControlAudioProcessor::midiIsSilenced() const
{
    for (auto* scope = midiSilence; scope != nullptr; scope = scope->previous)
        if (&scope->processor == this) return true;
    return false;
}

void BSIIControlAudioProcessor::clearPendingMidi()
{
    for (auto& value : pendingMidi) value.store (bsii::noHardwareValue);
    pendingTrigger.store (-1);
}

void BSIIControlAudioProcessor::parameterChanged (const juce::String& id, float newValue)
{
    const auto value = juce::roundToInt (newValue);
    const bool silent = midiIsSilenced();
    const auto& params = bsii::parameterSpecs();
    for (size_t i = 0; i < params.size(); ++i)
        if (id == params[i].id)
        {
            pendingMidi[i].store (silent ? bsii::noHardwareValue : value);
            return;
        }
    const auto& choices = bsii::choiceSpecs();
    for (size_t i = 0; i < choices.size(); ++i)
        if (id == choices[i].id)
        {
            if (id == "ampTrigger" || id == "modTrigger")
            {
                // Both controls represent the shared front-panel Triggering edit.
                // Imported patches retain their distinct stored trigger fields.
                if (! silent) pendingTrigger.store (value);
                else if (midiSilence->discardPendingTrigger) pendingTrigger.store (-1);
            }
            else pendingMidi[maxSpecs + i].store (silent ? bsii::noHardwareValue : value);
            return;
        }
}

void BSIIControlAudioProcessor::rememberHardwareMidiOutput (const juce::String& identifier,
                                                          const juce::String& name)
{
    const juce::ScopedLock lock (connectionLock);
    savedOutputIdentifier = identifier;
    savedOutputName = name;
    connectionSaved = true;
    connectionRestorePending = false;
}

bool BSIIControlAudioProcessor::hasSavedHardwareConnection() const
{
    const juce::ScopedLock lock (connectionLock);
    return connectionSaved;
}

void BSIIControlAudioProcessor::restoreHardwareConnection()
{
    // State may arrive on a host worker thread. Native port discovery/opening
    // stays on this message-thread timer, before dispatching pending automation.
    const juce::ScopedLock lock (connectionLock);
    if (! connectionRestorePending) return;
    const auto now = juce::Time::getMillisecondCounter();
    if (lastConnectionAttemptMs != 0 && now - lastConnectionAttemptMs < 1000u) return;
    lastConnectionAttemptMs = now;
    midiOutput.reset();
    connectHardwareSysExInput ({});
    if (savedOutputIdentifier.isEmpty() && savedOutputName.isEmpty())
    {
        connectionRestorePending = false;
        return;
    }
    const auto devices = juce::MidiOutput::getAvailableDevices();
    juce::String identifier;
    for (const auto& device : devices)
        if (device.identifier == savedOutputIdentifier) { identifier = device.identifier; break; }
    if (identifier.isEmpty() && savedOutputName.isNotEmpty())
    {
        // OS identifiers may change after reconnecting USB. Fall back only to
        // one exact name match; never choose a different or ambiguous device.
        int matches = 0;
        for (const auto& device : devices)
            if (device.name == savedOutputName) { identifier = device.identifier; ++matches; }
        if (matches != 1) identifier.clear();
    }
    if (identifier.isEmpty()) return; // Retry when the saved hardware appears.
    midiOutput = juce::MidiOutput::openDevice (identifier);
    if (! midiOutput) return;
    savedOutputIdentifier = identifier;
    savedOutputName = midiOutput->getName();
    connectHardwareSysExInput (savedOutputName);
    connectionRestorePending = false;
}

BSIIControlAudioProcessor::ProjectRecallInfo BSIIControlAudioProcessor::getProjectRecallInfo() const
{
    const juce::ScopedLock lock (connectionLock);
    return { recallMode, recallProgram, bsii::isRecallPatch (projectSound), capturePending, recallStatus };
}

bool BSIIControlAudioProcessor::configureProjectRecall (RecallMode mode, int program)
{
    const juce::ScopedLock lock (connectionLock);
    if (mode == RecallMode::preset && ! juce::isPositiveAndBelow (program, 128))
    { recallStatus = "Select a known preset first."; return false; }
    if (mode == RecallMode::capturedSound && ! bsii::isRecallPatch (projectSound))
    { recallStatus = "Capture the hardware sound first."; return false; }
    if (mode != RecallMode::off && mode != RecallMode::preset && mode != RecallMode::capturedSound) return false;
    recallMode = mode;
    if (mode == RecallMode::preset) recallProgram = program;
    recallPending = false; // Configuring recall never immediately replaces the sound.
    recallStatus = mode == RecallMode::off ? "Automatic recall is off."
        : mode == RecallMode::preset ? "Will load stored preset " + juce::String (recallProgram) + " when this Set reconnects."
        : "Will restore the captured sound to the edit buffer when this Set reconnects.";
    updateHostDisplay (juce::AudioProcessor::ChangeDetails().withNonParameterStateChanged (true));
    return true;
}

bool BSIIControlAudioProcessor::captureProjectSound()
{
    const juce::ScopedLock lock (connectionLock);
    if (capturePending || ! midiOutput) { recallStatus = "Capture needs a connected MIDI output."; return false; }
    if (usesDirectHardwareSysEx() && ! connectHardwareSysExInput (midiOutput->getName()))
    { recallStatus = "Capture needs one matching MIDI input."; return false; }
    captureAfterSerial = getLatestSysEx().serial;
    captureStartedMs = juce::Time::getMillisecondCounter();
    capturePending = true;
    static constexpr uint8_t request[] { 0xf0, 0, 0x20, 0x29, 0, 0x33, 0, 0x40, 0xf7 };
    midiOutput->sendMessageNow (juce::MidiMessage (request, sizeof (request), 0.0));
    recallStatus = "Capturing current hardware sound...";
    return true;
}

bool BSIIControlAudioProcessor::serviceProjectRecall()
{
    const auto latest = getLatestSysEx(); // Native receive callback only owns sysExLock.
    const auto now = juce::Time::getMillisecondCounter();
    const juce::ScopedLock lock (connectionLock);
    if (capturePending)
    {
        if (latest.serial != captureAfterSerial)
        {
            capturePending = false;
            if (bsii::isRecallPatch (latest.data))
            {
                projectSound = latest.data;
                recallStatus = "Project sound captured. Choose Captured sound, then save the Set.";
                updateHostDisplay (juce::AudioProcessor::ChangeDetails().withNonParameterStateChanged (true));
            }
            else recallStatus = "Capture rejected: expected one current edit-buffer patch. Previous capture kept.";
        }
        else if (now - captureStartedMs >= 3000u)
        { capturePending = false; recallStatus = "No capture reply. Previous project sound kept."; }
    }
    // Watch actual endpoint disappearance. Do not interpret ordinary Stop/Play
    // or editor creation as a reconnect, and never resend on each timer tick.
    if (now - lastPortCheckMs >= 1000u)
    {
        lastPortCheckMs = now;
        if (midiOutput && connectionSaved)
        {
            bool found = false;
            for (const auto& device : juce::MidiOutput::getAvailableDevices())
                found |= device.identifier == midiOutput->getIdentifier();
            if (! found)
            {
                midiOutput.reset();
                connectHardwareSysExInput ({});
                connectionRestorePending = true;
                lastConnectionAttemptMs = 0;
                recallPending = recallMode != RecallMode::off;
                recallSettling = false;
                capturePending = false;
                recallStatus = "Waiting for saved MIDI device.";
            }
        }
    }
    if (recallPending && midiOutput)
    {
        recallPending = false;
        if (recallMode == RecallMode::preset && juce::isPositiveAndBelow (recallProgram, 128))
        {
            midiOutput->sendMessageNow (juce::MidiMessage::programChange (hardwareMidiChannel.load(), recallProgram));
            knownHardwareProgram.store (recallProgram);
            recallStatus = "Restored stored preset " + juce::String (recallProgram) + ".";
        }
        else if (recallMode == RecallMode::capturedSound && bsii::isRecallPatch (projectSound))
        {
            midiOutput->sendMessageNow (juce::MidiMessage (projectSound.getData(), static_cast<int> (projectSound.getSize()), 0.0));
            knownHardwareProgram.store (-1);
            recallStatus = "Restored captured sound to edit buffer. Stored presets unchanged.";
        }
        else return false;
        // Drop old incoming controls/PCs from before recall, but preserve queued
        // host automation values so they are dispatched after the sound settles.
        pendingHardwareProgram.store (-1);
        for (auto& value : pendingHardwareValues) value.store (bsii::noHardwareValue);
        for (auto& value : pendingChoiceWireValues) value.store (-1);
        recallStartedMs = now;
        recallSettling = true;
    }
    if (recallSettling && now - recallStartedMs < 300u) return true;
    recallSettling = false;
    return false;
}

void BSIIControlAudioProcessor::timerCallback()
{
    restoreHardwareConnection();
    if (serviceProjectRecall()) return;
    const auto channel = hardwareMidiChannel.load();
    const auto& params = bsii::parameterSpecs();
    for (size_t i = 0; i < params.size(); ++i)
    {
        const auto value = pendingMidi[i].exchange (bsii::noHardwareValue);
        if (midiOutput && value != bsii::noHardwareValue)
            bsii::sendParameter (*midiOutput, channel, params[i], value);
    }
    const auto& choices = bsii::choiceSpecs();
    for (size_t i = 0; i < choices.size(); ++i)
    {
        const auto value = pendingMidi[maxSpecs + i].exchange (bsii::noHardwareValue);
        if (midiOutput && juce::isPositiveAndBelow (value, static_cast<int> (choices[i].choices.size())))
            bsii::sendChoice (*midiOutput, channel, choices[i], choices[i].choices[static_cast<size_t> (value)].wireValue);
    }
    const auto trigger = pendingTrigger.exchange (-1);
    if (trigger >= 0)
    {
        ScopedMidiSilence silence (*this, false);
        for (const auto& spec : choices)
            if ((juce::String (spec.id) == "ampTrigger" || juce::String (spec.id) == "modTrigger")
                && juce::isPositiveAndBelow (trigger, static_cast<int> (spec.choices.size())))
            {
                auto* parameter = apvts.getParameter (spec.id);
                parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (trigger)));
                if (midiOutput) bsii::sendChoice (*midiOutput, channel, spec, spec.choices[static_cast<size_t> (trigger)].wireValue);
            }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout
BSIIControlAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    using Parameter = juce::AudioProcessorValueTreeState::Parameter;
    using Attributes = juce::AudioProcessorValueTreeStateParameterAttributes;
    // APVTS::Parameter updates its adapter even for setValue(), allowing hardware
    // reception to update state without reporting a manual edit to the DAW.
    const auto addNumeric = [&] (const bsii::ParameterSpec& spec)
    {
        auto parameter = std::make_unique<Parameter> (
            juce::ParameterID { spec.id, 1 }, spec.name,
            juce::NormalisableRange<float> (static_cast<float> (spec.minimum), static_cast<float> (spec.maximum), 1.0f),
            static_cast<float> (spec.defaultValue), Attributes().withDiscrete (true)
                .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)); })
                .withValueFromStringFunction ([] (const juce::String& text) { return text.getFloatValue(); }));
        static_cast<juce::AudioProcessorParameter*> (parameter.get())->setValue (parameter->getDefaultValue());
        layout.add (std::move (parameter));
    };

    // Published layout: 58 original numeric controls, then 28 existing choices.
    // New numeric controls must follow those 86 parameters, not shift choices.
    const auto& numeric = bsii::parameterSpecs();
    const auto& discrete = bsii::choiceSpecs();
    constexpr size_t originalNumericCount = 58, originalChoiceCount = 28;
    for (size_t i = 0; i < originalNumericCount; ++i) addNumeric (numeric[i]);
    const auto addChoice = [&] (const bsii::ChoiceSpec& spec)
    {
        juce::StringArray choices;
        for (const auto& choice : spec.choices) choices.add (choice.label);
        auto parameter = std::make_unique<Parameter> (
            juce::ParameterID { spec.id, 1 }, spec.name,
            juce::NormalisableRange<float> (0.0f, static_cast<float> (choices.size() - 1), 1.0f),
            static_cast<float> (spec.defaultChoiceIndex), Attributes().withDiscrete (true)
                .withStringFromValueFunction ([choices] (float v, int)
                { return choices[juce::jlimit (0, choices.size() - 1, juce::roundToInt (v))]; })
                .withValueFromStringFunction ([choices] (const juce::String& text)
                { return static_cast<float> (juce::jmax (0, choices.indexOf (text))); }));
        static_cast<juce::AudioProcessorParameter*> (parameter.get())->setValue (parameter->getDefaultValue());
        layout.add (std::move (parameter));
    };
    for (size_t i = 0; i < originalChoiceCount; ++i) addChoice (discrete[i]);
    for (size_t i = originalNumericCount; i < numeric.size(); ++i) addNumeric (numeric[i]);
    for (size_t i = originalChoiceCount; i < discrete.size(); ++i) addChoice (discrete[i]);

    return layout;
}

void BSIIControlAudioProcessor::prepareToPlay (double, int) {}
void BSIIControlAudioProcessor::releaseResources() {}

bool BSIIControlAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
        && (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
            || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void BSIIControlAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (buffer);
    parseIncomingMidi (midiMessages, ! directSysExInput.isOpen());
    bsii::finishIncomingMidi (midiMessages, wrapperType == wrapperType_Standalone);
}

void BSIIControlAudioProcessor::publishNRPN (int channel, int nrpnMSB, int nrpnLSB, int value)
{
    lastNrpnMSB.store (nrpnMSB, std::memory_order_relaxed);
    lastNrpnLSB.store (nrpnLSB, std::memory_order_relaxed);
    lastNrpnValue.store (value, std::memory_order_relaxed);
    lastNrpnSerial.fetch_add (1, std::memory_order_release);

    if (channel != hardwareMidiChannel.load (std::memory_order_relaxed))
        return;

    const auto& params = bsii::parameterSpecs();
    for (size_t i = 0; i < params.size() && i < maxSpecs; ++i)
    {
        const auto& spec = params[i];
        if (spec.protocol == bsii::Protocol::nrpn7
            && spec.numberMSB == nrpnMSB && spec.numberLSB == nrpnLSB)
        {
            pendingHardwareValues[i].store (
                juce::jlimit (spec.minimum, spec.maximum, value), std::memory_order_release);
        }
    }

    const auto& choices = bsii::choiceSpecs();
    for (size_t i = 0; i < choices.size() && i < maxChoices; ++i)
    {
        const auto& spec = choices[i];
        if (spec.protocol == bsii::Protocol::nrpn7
            && spec.numberMSB == nrpnMSB && spec.numberLSB == nrpnLSB)
        {
            pendingChoiceWireValues[i].store (value, std::memory_order_release);
        }
    }
}

void BSIIControlAudioProcessor::receiveSysEx (const juce::MidiMessage& message)
{
    const juce::ScopedLock lock (sysExLock);
    lastSysExData = juce::MemoryBlock (message.getRawData(), static_cast<size_t> (message.getRawDataSize()));
    lastSysExSerial.fetch_add (1, std::memory_order_release);
}

void BSIIControlAudioProcessor::parseIncomingMidi (const juce::MidiBuffer& midiMessages, bool acceptSysEx, bool directControls)
{
    const auto wantedChannel = hardwareMidiChannel.load (std::memory_order_relaxed);
    const auto& params = bsii::parameterSpecs();
    const auto& choices = bsii::choiceSpecs();

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isSysEx())
        {
            // A host may also forward this reply. Prefer the direct port to
            // avoid processing the same dump twice.
            if (acceptSysEx) receiveSysEx (message);
            continue;
        }

        // One authoritative control source: direct port when connected, host
        // route otherwise (including standalone). Do not decode host duplicates.
        // Check per message so a port change cannot retain a whole stale block.
        if (directControls != directSysExInput.isOpen()) continue;
        if (! liveMidiRxEnabled.load (std::memory_order_relaxed))
            continue;

        if (message.isProgramChange() && message.getChannel() == wantedChannel)
        {
            for (auto& value : pendingHardwareValues) value.store (bsii::noHardwareValue);
            for (auto& value : pendingChoiceWireValues) value.store (-1);
            for (auto& value : pairedMSB) value.store (-1);
            knownHardwareProgram.store (message.getProgramChangeNumber());
            pendingHardwareProgram.store (message.getProgramChangeNumber(), std::memory_order_release);
            continue;
        }

        if (! message.isController())
            continue;

        const auto channel = message.getChannel();
        const auto controller = message.getControllerNumber();
        const auto value = message.getControllerValue();

        lastRxChannel.store (channel, std::memory_order_relaxed);
        lastRxController.store (controller, std::memory_order_relaxed);
        lastRxValue.store (value, std::memory_order_relaxed);
        lastRxSerial.fetch_add (1, std::memory_order_release);

        if (channel >= 1 && channel <= 16)
        {
            if (controller == 100 || controller == 101)
            {
                // An RPN selection cancels stale NRPN state on this channel.
                currentNrpnMSB[static_cast<size_t> (channel)].store (-1, std::memory_order_relaxed);
                currentNrpnLSB[static_cast<size_t> (channel)].store (-1, std::memory_order_relaxed);
                continue;
            }
            if (controller == 99)
            {
                currentNrpnMSB[static_cast<size_t> (channel)].store (value, std::memory_order_relaxed);
                continue;
            }
            if (controller == 98)
            {
                currentNrpnLSB[static_cast<size_t> (channel)].store (value, std::memory_order_relaxed);
                continue;
            }
            if (controller == 6)
            {
                const auto msb = currentNrpnMSB[static_cast<size_t> (channel)].load (std::memory_order_relaxed);
                const auto lsb = currentNrpnLSB[static_cast<size_t> (channel)].load (std::memory_order_relaxed);
                if (msb >= 0 && lsb >= 0)
                    publishNRPN (channel, msb, lsb, value);
                continue;
            }
        }

        if (channel != wantedChannel)
            continue;

        pairedMSB[static_cast<size_t> (controller)].store (value, std::memory_order_release);

        for (size_t i = 0; i < params.size() && i < maxSpecs; ++i)
        {
            const auto& spec = params[i];
            if (spec.protocol == bsii::Protocol::cc7)
            {
                if (controller == spec.numberMSB)
                {
                    const auto decoded = juce::String (spec.id) == "bendAmount"
                        ? bsii::bendAmountFromWire (value) : value;
                    if (juce::String (spec.id) == "bendAmount"
                        && (decoded < spec.minimum || decoded > spec.maximum))
                        continue;
                    pendingHardwareValues[i].store (
                        juce::jlimit (spec.minimum, spec.maximum, decoded),
                        std::memory_order_release);
                }
                continue;
            }

            if (spec.protocol == bsii::Protocol::pairedCC8 && controller == spec.numberLSB)
            {
                const auto msb = pairedMSB[static_cast<size_t> (spec.numberMSB)].load (std::memory_order_acquire);
                if (msb >= 0)
                {
                    const auto value8 = bsii::pairedCCToRaw (msb, value);
                    pendingHardwareValues[i].store (
                        juce::jlimit (spec.minimum, spec.maximum, value8),
                        std::memory_order_release);
                }
            }
        }

        for (size_t i = 0; i < choices.size() && i < maxChoices; ++i)
        {
            const auto& spec = choices[i];
            if (spec.protocol == bsii::Protocol::cc7 && controller == spec.numberMSB)
                pendingChoiceWireValues[i].store (value, std::memory_order_release);
        }
    }
}

void BSIIControlAudioProcessor::setHardwareMidiChannel (int channel)
{
    clearPendingMidi();
    if (hardwareMidiChannel.load() != juce::jlimit (1, 16, channel)) knownHardwareProgram.store (-1);
    hardwareMidiChannel.store (juce::jlimit (1, 16, channel), std::memory_order_relaxed);
    pendingHardwareProgram.store (-1);
}

int BSIIControlAudioProcessor::consumeHardwareValue (int specIndex)
{
    if (! juce::isPositiveAndBelow (specIndex, static_cast<int> (maxSpecs))) return bsii::noHardwareValue;
    return pendingHardwareValues[static_cast<size_t> (specIndex)].exchange (bsii::noHardwareValue, std::memory_order_acq_rel);
}

int BSIIControlAudioProcessor::consumeChoiceWireValue (int choiceIndex)
{
    if (! juce::isPositiveAndBelow (choiceIndex, static_cast<int> (maxChoices))) return -1;
    return pendingChoiceWireValues[static_cast<size_t> (choiceIndex)].exchange (-1, std::memory_order_acq_rel);
}

BSIIControlAudioProcessor::ReceivedMidi BSIIControlAudioProcessor::getLatestReceivedMidi() const
{
    ReceivedMidi result;
    result.serial = lastRxSerial.load (std::memory_order_acquire);
    result.channel = lastRxChannel.load (std::memory_order_relaxed);
    result.controller = lastRxController.load (std::memory_order_relaxed);
    result.value = lastRxValue.load (std::memory_order_relaxed);
    result.nrpnSerial = lastNrpnSerial.load (std::memory_order_acquire);
    result.nrpnMSB = lastNrpnMSB.load (std::memory_order_relaxed);
    result.nrpnLSB = lastNrpnLSB.load (std::memory_order_relaxed);
    result.nrpnValue = lastNrpnValue.load (std::memory_order_relaxed);
    return result;
}

BSIIControlAudioProcessor::ReceivedSysEx BSIIControlAudioProcessor::getLatestSysEx() const
{
    ReceivedSysEx result;
    const juce::ScopedLock lock (sysExLock);
    result.data = lastSysExData;
    result.serial = lastSysExSerial.load (std::memory_order_acquire);
    return result;
}

juce::AudioProcessorEditor* BSIIControlAudioProcessor::createEditor()
{
    return new BSIIControlAudioProcessorEditor (*this);
}

void BSIIControlAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    {
        const juce::ScopedLock lock (connectionLock);
        state.setProperty ("bsiiConnectionSaved", connectionSaved, nullptr);
        state.setProperty ("bsiiMidiOutputId", savedOutputIdentifier, nullptr);
        state.setProperty ("bsiiMidiOutputName", savedOutputName, nullptr);
        state.setProperty ("bsiiMidiChannel", hardwareMidiChannel.load(), nullptr);
        state.setProperty ("bsiiLiveRxEnabled", liveMidiRxEnabled.load(), nullptr);
        state.setProperty ("bsiiRecallMode", static_cast<int> (recallMode), nullptr);
        state.setProperty ("bsiiRecallProgram", recallProgram, nullptr);
        state.setProperty ("bsiiProjectSound", projectSound.toBase64Encoding(), nullptr);
    }
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void BSIIControlAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (! xml) return;
    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid() || ! state.hasType (apvts.state.getType())) return;
    ScopedMidiSilence silence (*this);
    clearPendingMidi();
    if (static_cast<bool> (state.getProperty ("bsiiConnectionSaved", false)))
    {
        const juce::ScopedLock lock (connectionLock);
        savedOutputIdentifier = state.getProperty ("bsiiMidiOutputId").toString();
        savedOutputName = state.getProperty ("bsiiMidiOutputName").toString();
        connectionSaved = true;
        connectionRestorePending = true;
        lastConnectionAttemptMs = 0;
        setHardwareMidiChannel (static_cast<int> (state.getProperty ("bsiiMidiChannel", 1)));
        setLiveMidiRxEnabled (static_cast<bool> (state.getProperty ("bsiiLiveRxEnabled", true)));
    }
    {
        const juce::ScopedLock lock (connectionLock);
        recallMode = RecallMode::off;
        recallProgram = static_cast<int> (state.getProperty ("bsiiRecallProgram", -1));
        projectSound.reset();
        const auto encoded = state.getProperty ("bsiiProjectSound").toString();
        if (encoded.length() <= 1024)
        {
            juce::MemoryBlock patch;
            if (patch.fromBase64Encoding (encoded) && bsii::isRecallPatch (patch)) projectSound = patch;
        }
        const auto requestedMode = static_cast<int> (state.getProperty ("bsiiRecallMode", 0));
        if (requestedMode == 1 && juce::isPositiveAndBelow (recallProgram, 128)) recallMode = RecallMode::preset;
        if (requestedMode == 2 && bsii::isRecallPatch (projectSound)) recallMode = RecallMode::capturedSound;
        recallPending = recallMode != RecallMode::off;
        recallSettling = false;
        capturePending = false;
        recallStatus = recallPending ? "Waiting to restore project sound on connection." : "Automatic recall is off.";
    }
    // Connection metadata belongs to host sessions, not portable patch snapshots.
    for (const auto* property : { "bsiiConnectionSaved", "bsiiMidiOutputId", "bsiiMidiOutputName",
                                 "bsiiMidiChannel", "bsiiLiveRxEnabled", "bsiiRecallMode", "bsiiRecallProgram", "bsiiProjectSound" })
        state.removeProperty (property, nullptr);
    apvts.replaceState (state);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BSIIControlAudioProcessor();
}
