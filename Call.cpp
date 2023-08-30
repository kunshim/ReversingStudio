#include "Call.h"
#include "Image.h"

std::tr1::unordered_map<ADDRINT, std::string> Call::nameCache;

Call::Call(ADDRINT addr, ADDRINT sfp): addr(addr), sfp(sfp), endFlag(false), parent(NULL)
{        
    isSystemCall = Image::getImageByAddress(addr).isSystemImage();
    if (nameCache.find(addr) != nameCache.end())
        desc = nameCache[addr];
    else
    {
        std::string funcName = "";
        std::string imgName = "";
        PIN_LockClient();    
        imgName = Image::getImageByAddress(addr).getShortName();        
        funcName = RTN_FindNameByAddress(addr);
        if (funcName == ".text" || funcName == "unnamedImageEntryPoint")
        {
            std::stringstream hexStream;
            hexStream << std::hex << addr;
            funcName = hexStream.str();
        }
        PIN_UnlockClient();
        if (funcName == "")
            funcName = "Undefined";
        desc = imgName + "!" + funcName;
        nameCache[addr] = desc;
    }
    analysisABI();
}

void Call::analysisABI()
{
#ifdef TARGET_IA32
    abi = ABI::ABI_STDCALL;
#elif TARGET_IA32e
    abi = ABI::ABI_FASTCALL;
#endif
    RTN rtn = RTN_FindByAddress(addr);
    if (RTN_Invalid(rtn))
    {
        std::stringstream ss;
        ss << std::hex << addr;
        PIN_LockClient();
        rtn = RTN_CreateAt(rtn, ss.str());
        PIN_UnlockClient();
        for(INS ins = RTN_InsHead(); !INS_IsControlFlow(ins); ins = INS_Next(ins))
        {

        }
    }
}

void Call::setEnd()
{
    //std::cout << "setEnd " << std::hex << addr << std::endl;
    endFlag = true;
}

bool Call::isEnd()
{
    return endFlag;
}

void Call::descript()
{
    for(auto const& branch : branches)
    {
        std::cout << "Branch : " << std::hex << branch->addr << " (" << branch->desc << ")\n";
        branch->descript();
    }
    for(auto const& child : childs)
    {
        std::cout << "──────────────────────────────────────────────────────────\n";
        std::cout << "Call : " << std::hex << child->addr << " (" << child->desc << ")\n";
        if (child->parent)
            std::cout << "Parent : " << std::hex << child->parent->addr << " (" << child->parent->desc << ")\n";
        std::cout << "──────────────────────────────────────────────────────────\n";
        child->descript();
    }
}

Call::~Call()
{
    ASSERT(addr != NULL, "Dobule-free detected");
    addr = NULL;
    for(auto const& branch : branches)
        delete branch;
    for(auto const& child : childs)
        delete child;
}