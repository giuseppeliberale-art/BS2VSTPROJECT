#include "BSIITunImport.h"
#include "ThirdParty/anamark/TUN_Formula.h"
#include <algorithm>
#include <charconv>
#include <set>

namespace TUN { const long MaxNumOfNotes = 128; }

namespace bsii::microtuning::tun
{
namespace
{
struct Record { std::string key, value; int line = 0; };
using Records = std::vector<Record>;
std::string quoted (const std::string& text)
{
    if (text.size()<2 || text.front()!='"' || text.back()!='"') throw std::runtime_error("Expected a quoted TUN string.");
    std::string result;
    const auto hex = [] (char c) -> int { if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1; };
    for (std::size_t i=1;i+1<text.size();++i)
    {
        char c=text[i];
        if(c=='"')throw std::runtime_error("Unexpected quote in TUN string.");
        if(c=='\\')
        {
            if(++i>=text.size()-1)throw std::runtime_error("Incomplete TUN string escape.");
            switch(c=text[i])
            {
                case 'n':c='\n';break;case 'r':c='\r';break;case 't':c='\t';break;
                case 'a':c='\a';break;case 'b':c='\b';break;case 'f':c='\f';break;case 'v':c='\v';break;
                case '\\':case '"':case '\'':case '?':break;
                case '0':
                    if(i+3<text.size()-1 && text[i+1]=='x' && hex(text[i+2])>=0 && hex(text[i+3])>=0)
                    {c=static_cast<char>(hex(text[i+2])*16+hex(text[i+3]));i+=3;}
                    else c='\0';
                    break;
                case 'x':
                {
                    int value=0, count=0;
                    const int limit=i+1<text.size()-1 && text[i+1]=='0'?3:2;
                    while(count<limit && i+1<text.size()-1 && hex(text[i+1])>=0)
                    {value=value*16+hex(text[++i]);++count;}
                    if(count==0 || value>255)throw std::runtime_error("Invalid TUN hexadecimal escape.");
                    c=static_cast<char>(value);break;
                }
                default:throw std::runtime_error("Unknown TUN string escape.");
            }
        }
        result+=c;
    }
    return result;
}
int index (const std::string& key, const std::string& prefix)
{
    if(key==prefix)throw std::runtime_error("Missing note/key index.");
    const auto suffix=scala::trim(key.substr(prefix.size()));
    return integer(suffix,0,127);
}
// Validate and bound every parameter before calling the upstream formula engine.
// No host expressions, callbacks, recursion, file operations or code evaluation.
std::string checkedFormula (const std::string& text)
{
    auto formula=quoted(text);
    formula.erase(std::remove_if(formula.begin(),formula.end(),[](char c){return c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\v'||c=='\f';}),formula.end());
    std::size_t pos=0;
    while(pos<formula.size())
    {
        const auto token=formula[pos++];
        if(std::string("!#*/%+~").find(token)==std::string::npos)throw std::runtime_error("Unknown formula token.");
        bool reference=false;
        if((token=='#'||token=='+') && pos<formula.size() && (formula[pos]=='='||formula[pos]=='>')) {reference=true;++pos;}
        const auto start=pos;
        if(pos<formula.size() && formula[pos]=='+')++pos;
        if(reference||token=='~')
        {
            long value=0;
            const auto parsed=std::from_chars(formula.data()+pos,formula.data()+formula.size(),value);
            if(parsed.ec!=std::errc{} || parsed.ptr==formula.data()+pos)throw std::runtime_error("Invalid integer formula parameter.");
            pos=static_cast<std::size_t>(parsed.ptr-formula.data());
            if((reference && (value < -127 || value > 127)) || (!reference && (value < -1000000 || value > 1000000)))
                throw std::runtime_error("Formula reference/loop outside supported range.");
            // An absolute reference cannot name a negative scale note.
            if(reference && formula[start-1]=='=' && value<0)throw std::runtime_error("Negative absolute note reference.");
        }
        else
        {
            double value=0;
            const auto parsed=std::from_chars(formula.data()+pos,formula.data()+formula.size(),value);
            if(parsed.ec!=std::errc{} || parsed.ptr==formula.data()+pos || !std::isfinite(value))throw std::runtime_error("Invalid finite formula parameter.");
            pos=static_cast<std::size_t>(parsed.ptr-formula.data());
            if(std::abs(value)>(token=='%'?120000.0:1.0e9))throw std::runtime_error("Formula parameter outside supported range.");
            if(token=='/' && value==0)throw std::runtime_error("Formula divisor is zero.");
            if(token=='!' && (value<=0 || start!=1 || pos!=formula.size()))throw std::runtime_error("Ensure-frequency must be positive and the only formula token.");
        }
    }
    return formula;
}
void initEqual (std::vector<double>& notes, const Record& record)
{
    const auto value=scala::trim(record.value);
    if(value.size()<5 || value.front()!='(' || value.back()!=')')throw std::runtime_error("InitEqual requires (reference note, Hz).");
    const auto params=value.substr(1,value.size()-2);const auto comma=params.find(',');
    if(comma==std::string::npos)throw std::runtime_error("InitEqual needs two parameters.");
    const auto note=integer(scala::trim(params.substr(0,comma)),0,127);
    const auto hz=scala::number(scala::trim(params.substr(comma+1)));
    if(hz<0.000001 || hz>1e9)throw std::runtime_error("InitEqual reference outside supported range.");
    for(int key=0;key<128;++key)notes[key]=hz*std::exp2((key-note)/12.0);
}
}
scala::Preview parseV2 (const std::string& input, const std::string& fallbackName)
{
    scala::Preview result;
    try
    {
        if(input.empty() || input.size()>65536)throw std::runtime_error("Use a nonempty TUN file up to 64 KiB.");
        std::string text=input;
        if(text.starts_with("\xef\xbb\xbf"))text.erase(0,3);
        for(auto& c:text)if(c=='\r'||c=='\0')c='\n';
        std::istringstream stream(text);std::string section;std::set<std::string> seen;
        Records begin,info,functions,mapping;
        bool started=false,ended=false;int line=0;
        for(std::string row;std::getline(stream,row);)
        {
            ++line;row=scala::trim(uncomment(row));if(row.empty())continue;
            if(row.front()=='[')
            {
                if(row.back()!=']')throw std::runtime_error("Malformed TUN section header.");
                section=lower(scala::trim(row.substr(1,row.size()-2)));
                if(section=="scale begin")
                {
                    if(started)throw std::runtime_error("Multiple-scale files are not supported.");
                    started=true;
                }
                if(!started)throw std::runtime_error("TUN v2 requires Scale Begin before data sections.");
                if(ended)throw std::runtime_error("Additional sections after Scale End are not supported.");
                if(section=="scale end"){ended=true;continue;}
                if(section=="scale begin"||section=="info"||section=="functional tuning"||section=="mapping")
                    if(!seen.insert(section).second)throw std::runtime_error("Repeated TUN v2 section.");
                continue;
            }
            if(!started||ended)continue;
            // V2 formulas are authoritative. Numeric compatibility, assignments
            // and editor metadata do not alter a single flattened local table.
            Records* target=section=="scale begin"?&begin:section=="info"?&info:section=="functional tuning"?&functions:section=="mapping"?&mapping:nullptr;
            if(!target)continue;
            const auto equal=row.find('=');
            if(equal==std::string::npos)throw std::runtime_error("Malformed TUN v2 assignment at line "+std::to_string(line));
            target->push_back({lower(scala::trim(row.substr(0,equal))),scala::trim(row.substr(equal+1)),line});
            if(functions.size()+mapping.size()+begin.size()+info.size()>4096)throw std::runtime_error("Too many TUN v2 operations (maximum 4096).");
        }
        if(!started||!ended||!seen.count("info")||!seen.count("functional tuning"))throw std::runtime_error("TUN v2 needs Scale Begin, Info, Functional Tuning and Scale End.");
        bool version=false;result.name=fallbackName.empty()?"Imported TUN table":fallbackName;
        for(const auto& record:begin)
        {
            if(record.key=="format" && quoted(record.value)!="AnaMark-TUN")throw std::runtime_error("Unknown TUN format.");
            if(record.key=="formatversion") {if(integer(record.value,0,65535)!=200)throw std::runtime_error("Expected TUN version 200.");version=true;}
        }
        if(!version)throw std::runtime_error("TUN v2 requires FormatVersion=200.");
        for(const auto& record:info)if(record.key=="name")
        {
            auto name=quoted(record.value);
            for(auto& c:name)if(static_cast<unsigned char>(c)<32)c=' ';
            name=scala::trim(name);if(!name.empty())result.name=name;
        }
        std::vector<double> notes(128);
        initEqual(notes,{"initequal","(69,440)",0});
        for(const auto& record:functions)
        {
            try
            {
                if(record.key=="initequal")initEqual(notes,record);
                else if(record.key=="note"||record.key.starts_with("note ")||record.key.starts_with("note\t"))
                {
                    const auto key=index(record.key,"note");
                    TUN::CFormula formula(key);
                    if(!formula.SetFromStr(checkedFormula(record.value)))throw std::runtime_error("Invalid formula.");
                    formula.Apply(notes);
                }
                for(const auto hz:notes)if(!std::isfinite(hz))throw std::runtime_error("Formula produced a non-finite frequency.");
            }
            catch(const std::exception& error){throw std::runtime_error("Line "+std::to_string(record.line)+": "+error.what());}
        }
        std::array<int,128> keys;for(int key=0;key<128;++key)keys[key]=key;
        int loop=0;
        for(const auto& record:mapping)
        {
            if(record.key=="loopsize")loop=integer(record.value,-1000000,1000000);
            else if(record.key=="keyboard"||record.key.starts_with("keyboard ")||record.key.starts_with("keyboard\t"))
                keys[index(record.key,"keyboard")]=integer(record.value,-1000000,1000000);
        }
        result.sourceInfo="TUN v2 formulas and keyboard mapping evaluated from the file.";
        for(int key=0;key<128;++key)
        {
            const auto scaleKey=loop>0?keys[key%loop]+(key/loop)*loop:keys[key];
            auto& note=result.notes[key];
            // Do not use the upstream map getter's boundary clamping: invalid
            // scale indices are explicitly unavailable in this complete table.
            if(scaleKey<0||scaleKey>=128){note.status=scala::NoteStatus::unmapped;continue;}
            note.hz=notes[scaleKey];
            if(note.hz<=0){note.status=scala::NoteStatus::muted;continue;}
            note.pitch=69+12*std::log2(note.hz/440);
            if(std::abs(note.pitch)<1e-10)note.pitch=0;
            if(std::abs(note.pitch-127)<1e-10)note.pitch=127;
            note.status=encodePitch(note.pitch)?scala::NoteStatus::available:scala::NoteStatus::outsidePitchRange;
            if(note.status==scala::NoteStatus::available)++result.available;
        }
    }
    catch(const std::exception& error){result.error=error.what();}
    return result;
}
}
