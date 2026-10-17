#include "BSIIPairedCC.h"
#include "BSIIMidiRouting.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

BSIIControlAudioProcessor::BSIIControlAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createParameterLayout()),
      directSysExInput ([this] (const juce::MidiMessage& message) { receiveSysEx (message); })
{
    for (auto& value : pendingHardwareValues) value.store (bsii::noHardwareValue, std::memory_order_relaxed);
    for (auto& value : pendingChoiceWireValues) value.store (-1, std::memory_order_relaxed);
    for (auto& value : pairedMSB) value.store (-1, std::memory_order_relaxed);
    for (auto& value : currentNrpnMSB) value.store (-1, std::memory_order_relaxed);
    for (auto& value : currentNrpnLSB) value.store (-1, std::memory_order_relaxed);
}

juce::AudioProcessorValueTreeState::ParameterLayout
BSIIControlAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (const auto& spec : bsii::parameterSpecs())
    {
        layout.add (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { spec.id, 1 }, spec.name,
            spec.minimum, spec.maximum, spec.defaultValue));
    }

    for (const auto& spec : bsii::choiceSpecs())
    {
        juce::StringArray choices;
        for (const auto& choice : spec.choices)
            choices.add (choice.label);

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { spec.id, 1 }, spec.name, choices,
            juce::jlimit (0, juce::jmax (0, choices.size() - 1), spec.defaultChoiceIndex)));
    }

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

void BSIIControlAudioProcessor::parseIncomingMidi (const juce::MidiBuffer& midiMessages, bool acceptSysEx)
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
            // avoid processing the same dump twice; CC/program reception stays hosted.
            if (acceptSysEx) receiveSysEx (message);
            continue;
        }

        if (! liveMidiRxEnabled.load (std::memory_order_relaxed))
            continue;

        if (message.isProgramChange() && message.getChannel() == wantedChannel)
        {
            for (auto& value : pendingHardwareValues) value.store (bsii::noHardwareValue);
            for (auto& value : pendingChoiceWireValues) value.store (-1);
            for (auto& value : pairedMSB) value.store (-1);
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
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void BSIIControlAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BSIIControlAudioProcessor();
}
