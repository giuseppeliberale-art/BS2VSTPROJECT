#pragma once
#include "PluginProcessor.h"
#include <functional>

class BankPage : public juce::Component, private juce::TableListBoxModel, private juce::Timer
{
public:
    BankPage (BSIIControlAudioProcessor&, std::function<bool(const juce::MemoryBlock&)>);
    ~BankPage() override;
    void resized() override;
    bool loadBankFile (const juce::File&);
    bool loadSelectedTemporary();
private:
    void chooseBank();
    void refresh();
    void timerCallback() override { refresh(); }
    int getNumRows() override { return shown ? 128 : 0; }
    void paintRowBackground(juce::Graphics&,int,int,int,bool) override;
    void paintCell(juce::Graphics&,int,int,int,int,bool) override;
    void selectedRowsChanged(int) override;
    BSIIControlAudioProcessor& processor;
    std::function<bool(const juce::MemoryBlock&)> send;
    std::shared_ptr<const bsii::PatchBank> shown;
    juce::Viewport viewport;
    juce::Component content;
    juce::Label heading, hint, source, status;
    juce::TextButton open { "Open Bank..." }, load { "Load Selected to BSII" };
    juce::TableListBox table { "Bank entries", this };
    std::unique_ptr<juce::FileChooser> chooser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankPage)
};
