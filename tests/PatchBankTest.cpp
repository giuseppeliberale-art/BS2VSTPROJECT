#include "BSIIPatchBank.h"
#include <iostream>
#include <cstring>
int main(int argc,char** argv)
{
    if (argc!=2) return 2;
    int failures=0;
    auto check=[&](bool ok,const char* text){ if(!ok){++failures;std::cerr<<"FAIL: "<<text<<'\n';} };
    juce::MemoryBlock fixture;
    if (!juce::File(juce::String::fromUTF8(argv[1])).loadFileAsData(fixture) || !bsii::isRecallPatch(fixture)) return 2;
    std::array<juce::MemoryBlock,128> entries;
    juce::MemoryBlock raw;
    for (int i=127;i>=0;--i)
    {
        auto patch=fixture;auto* b=static_cast<juce::uint8*>(patch.getData());
        b[7]=1;b[8]=static_cast<juce::uint8>(i);b[20]=static_cast<juce::uint8>(i);
        std::fill(b+137,b+153,' ');
        const auto name="Entry "+juce::String(i);
        std::memcpy(b+137,name.toRawUTF8(),static_cast<size_t>(name.getNumBytesAsUTF8()));
        entries[static_cast<size_t>(i)]=patch;raw.append(patch.getData(),patch.getSize());
    }
    juce::String error;
    auto bank=bsii::PatchBank::parse(raw,"synthetic.syx",error);
    check(bank!=nullptr && error.isEmpty(),"complete reordered bank accepted");
    if (!bank) return 1;
    check(bank->source()=="synthetic.syx","source label retained");
    for(int i=0;i<128;++i)
    {
        auto expected=entries[static_cast<size_t>(i)];auto* b=static_cast<juce::uint8*>(expected.getData());
        std::fill(b+6,b+13,0);
        check(bank->temporaryPatch(i)==expected,"full payload preserved with exact temporary header");
        check(bsii::isRecallPatch(bank->temporaryPatch(i)),"every extracted entry passes production gate");
        check(bank->name(i)=="Entry "+juce::String(i),"names indexed independently of stream order");
    }
    check(bank->temporaryPatch(-1).isEmpty() && bank->temporaryPatch(128).isEmpty(),"invalid indices rejected");
    auto reject=[&](const juce::MemoryBlock& data){check(!bsii::PatchBank::parse(data,"bad",error) && error.isNotEmpty(),"bad bank rejects atomically");};
    reject(juce::MemoryBlock());reject(fixture);reject(juce::MemoryBlock(raw.getData(),raw.getSize()-1));
    auto bad=raw;const juce::uint8 extra=0;bad.append(&extra,1);reject(bad);
    for(const auto offset : {0,5,6,7,9,10,11,12,20,153})
    {
        bad=raw;auto* b=static_cast<juce::uint8*>(bad.getData());b[offset]=static_cast<juce::uint8>(offset==0?0xf1:offset==5?0x34:offset==7||offset==153?0:offset==20?0x80:1);reject(bad);
    }
    bad=raw;static_cast<juce::uint8*>(bad.getData())[8]=126;reject(bad); // Duplicate index.
    bad=raw;static_cast<juce::uint8*>(bad.getData())[8]=128;reject(bad);
    const auto file=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("bsii-bank-test",".syx");
    check(file.replaceWithData(raw.getData(),raw.getSize()),"test file created");
    check(bsii::PatchBank::readFile(file,error)!=nullptr,"valid file read");
    bad=raw;bad.append(&extra,1);check(file.replaceWithData(bad.getData(),bad.getSize()),"oversize fixture written");
    check(!bsii::PatchBank::readFile(file,error),"bounded read detects trailing data");
    file.deleteFile();
    check(!bsii::PatchBank::readFile(file,error),"missing file rejected");
    std::cout<<(failures==0?"PASS":"FAIL")<<" strict bank parsing and temporary-patch extraction ("<<failures<<" failures)\n";
    return failures==0?0:1;
}
