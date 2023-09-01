#include "Call.h"
#include "Image.h"

std::tr1::unordered_map<ADDRINT, std::string> Call::nameCache;
std::tr1::unordered_map<ADDRINT, Call::ABI> Call::abiCache;
const char* Call::ABISTR[3] = {"__thiscall", "__stdcall", "__fastcall"};
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
#ifdef TARGET_IA32
    abi = ABI::ABI_STDCALL;
#elif TARGET_IA32e
    abi = ABI::ABI_FASTCALL;
#endif
    if (sfp)
        analysisABI();
}

void Call::setEnd()
{
    endFlag = true;
}

bool Call::isEnd()
{
    return endFlag;
}

bool Call::isCall()
{
    return callFlag;
}

void Call::analysisABI()
{
    if (abiCache.find(addr) != abiCache.end())
    {
        abi = abiCache[addr];
        return;
    }
    std::stringstream ss;
    ss << std::hex << addr;
    PIN_LockClient();
    RTN rtn = RTN_CreateAt(addr, ss.str());
    PIN_UnlockClient();
    RTN_Open(rtn);
    for(INS ins = RTN_InsHead(rtn); !INS_Valid(ins) || !INS_IsControlFlow(ins); ins = INS_Next(ins))
    {        
        if (INS_OperandCount(ins) >= 2)
        { 
            //ecx 레지스터가 변경되면 thiscall 은 아니다.
            if (INS_OperandIsReg(ins, 0) && INS_OperandReg(ins, 0) == REG_ECX && INS_OperandWritten(ins, 0))
                break;
            //ecx 에서 읽어서 다른 레지스터 또는 메모리로 값을 가지고 오는 경우
            if (INS_IsMov(ins) && INS_OperandIsReg(ins, 1) && INS_OperandReg(ins, 1) == REG_ECX && INS_OperandReadOnly(ins, 1) )
            {
                abi = ABI::ABI_THISCALL;
                break;
            }
        }
    }
    abiCache[addr] = abi;
    RTN_Close(rtn);
}



void Call::descript()
{
    for(auto const& branch : branches)
    {
        std::cout << "Branch : " << std::hex << branch->addr << " (" << branch->desc << ")\n";;
        branch->descript();
    }
    for(auto const& child : childs)
    {
        std::cout << "\nCall : " << ABISTR[child->abi] << " " << std::hex << child->addr << " (" << child->desc << ")\n";
        if (child->parent)
            std::cout << "Parent : " << std::hex << child->parent->addr << " (" << child->parent->desc << ")\n";
        child->descript();
    }
}

Call::~Call()
{
    addr = NULL;
    for(auto const& branch : branches)
        delete branch;
    for(auto const& child : childs)
        delete child;
}
