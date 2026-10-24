#include "BSIIBankPage.h"

BankPage::BankPage(BSIIControlAudioProcessor& p, std::function<bool(const juce::MemoryBlock&)> sender)
    : processor(p), send(std::move(sender))
{
    viewport.setViewedComponent(&content,false);
    viewport.setScrollBarsShown(true,false);
    addAndMakeVisible(viewport);
    for (auto* c : std::initializer_list<juce::Component*>{&heading,&hint,&source,&status,&open,&load,&table})
        content.addAndMakeVisible(c);
    heading.setText("Patch Banks",juce::dontSendNotification);
    heading.setFont(juce::FontOptions(18.0f).withStyle("Bold"));
    hint.setText("Opening and selecting send nothing. Load replaces the temporary sound; capture unsaved edits first. Stop playback before loading.",juce::dontSendNotification);
    hint.setColour(juce::Label::textColourId,juce::Colours::lightblue);
    hint.setMinimumHorizontalScale(1.0f);
    source.setTooltip("This bank stays open for this plugin instance but is not embedded in the DAW Set. Keep the original .syx file.");
    status.setText("No stored-preset writes are available here.",juce::dontSendNotification);
    status.setMinimumHorizontalScale(1.0f);
    table.getHeader().addColumn("Entry",1,70,55,100);
    table.getHeader().addColumn("Patch name",2,450,120,1000);
    table.getHeader().setStretchToFitActive(true);
    table.setMultipleSelectionEnabled(false);
    table.setRowHeight(28);
    table.setColour(juce::ListBox::backgroundColourId,juce::Colour(0xff202326));
    open.onClick = [this]{ chooseBank(); };
    load.onClick = [this]{ loadSelectedTemporary(); };
    refresh();
    startTimerHz(5);
}
BankPage::~BankPage()
{
    stopTimer(); chooser.reset(); table.setModel(nullptr); viewport.setViewedComponent(nullptr,false);
}
void BankPage::resized()
{
    viewport.setBounds(getLocalBounds());
    const int height=juce::jmax(360,getHeight());
    content.setSize(juce::jmax(1,getWidth()-(height>getHeight()?viewport.getScrollBarThickness():0)),height);
    auto area=content.getLocalBounds().reduced(12);
    heading.setBounds(area.removeFromTop(26));
    hint.setBounds(area.removeFromTop(46));
    auto buttons=area.removeFromTop(34); open.setBounds(buttons.removeFromLeft(145).reduced(0,2));
    buttons.removeFromLeft(10); load.setBounds(buttons.removeFromLeft(210).reduced(0,2));
    source.setBounds(area.removeFromTop(28));
    auto footer=area.removeFromBottom(46); table.setBounds(area); status.setBounds(footer);
}
void BankPage::refresh()
{
    auto bank=processor.getLocalBank();
    if (bank!=shown)
    {
        shown=std::move(bank); table.updateContent();
        source.setText(shown ? shown->source()+" | 128 patches" : "No bank open",juce::dontSendNotification);
    }
    const int selected=processor.getLocalBankSelection();
    if (table.getSelectedRow()!=selected) table.selectRow(selected);
    load.setEnabled(shown && juce::isPositiveAndBelow(selected,128) && processor.hardwareMidiOutput()!=nullptr);
}
bool BankPage::loadBankFile(const juce::File& file)
{
    juce::String error;
    auto bank=bsii::PatchBank::readFile(file,error);
    if (!bank) { status.setText(error,juce::dontSendNotification); return false; }
    processor.setLocalBank(std::move(bank));
    refresh();
    status.setText("Bank opened locally. Choose an entry, then Load Selected to BSII. Stored presets are not overwritten.",juce::dontSendNotification);
    return true;
}
void BankPage::chooseBank()
{
    chooser=std::make_unique<juce::FileChooser>("Open a complete Bass Station II bank",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),"*.syx");
    juce::Component::SafePointer<BankPage> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
        [safe](const juce::FileChooser& chosen)
        {
            const auto file=chosen.getResult();
            if (safe && file!=juce::File{}) safe->loadBankFile(file);
            if (safe) safe->chooser.reset();
        });
}
bool BankPage::loadSelectedTemporary()
{
    const auto bank=processor.getLocalBank();
    const auto index=processor.getLocalBankSelection();
    if (!bank || !juce::isPositiveAndBelow(index,128) || !processor.hardwareMidiOutput())
    { status.setText("Not sent. Open a bank, select an entry and connect a MIDI output.",juce::dontSendNotification); return false; }
    auto patch=bank->temporaryPatch(index);
    if (!bsii::isRecallPatch(patch) || !send(patch))
    { status.setText("Not sent. Check the MIDI output and selected patch.",juce::dontSendNotification); return false; }
    status.setText("Sent entry "+juce::String(index)+" to the temporary buffer. Stored presets unchanged; receipt not confirmed.",juce::dontSendNotification);
    return true;
}
void BankPage::selectedRowsChanged(int row)
{
    processor.selectLocalBankEntry(row);
    load.setEnabled(shown && juce::isPositiveAndBelow(row,128) && processor.hardwareMidiOutput()!=nullptr);
}
void BankPage::paintRowBackground(juce::Graphics& g,int row,int,int,bool selected)
{ g.fillAll(selected ? juce::Colour(0xff177b99) : (row%2?juce::Colour(0xff272b2e):juce::Colour(0xff202326))); }
void BankPage::paintCell(juce::Graphics& g,int row,int column,int width,int height,bool)
{
    if (!shown) return;
    g.setColour(juce::Colours::white);
    const auto name=shown->name(row);
    g.drawText(column==1?juce::String(row):(name.isNotEmpty()?name:"(unnamed)"),6,0,width-12,height,juce::Justification::centredLeft,true);
}
