#include "Call.h"
#include "Image.h"

std::unordered_map<ADDRINT, std::string> Call::nameCache;
std::unordered_map<ADDRINT, Call::ABI> Call::abiCache;
std::unordered_map<ADDRINT, bool> Call::stackCache;
const char* Call::ABISTR[6] = { "__thiscall", "__stdcall", "__fastcall", "__cdecl", "__usercall", "errorcall"};
Call::Call(ADDRINT addr, ADDRINT sfp, ADDRINT callSite, Call* parent, bool indirect) : addr(addr), sfp(sfp), parent(parent), indirect(indirect), callSite(callSite), endFlag(false), noStackFrame(true)
{
    Image tmp = Image::getImageByAddress(addr);
    systemCall = tmp.isSystemImage();
    relativeAddr = addr - tmp.getBaseAddress();
    if (nameCache.find(addr) != nameCache.end())
    {
        desc = nameCache[addr];
        abi = abiCache[addr];
        noStackFrame = stackCache[addr];
    }
    else if (isBranch())
    {
        desc = tmp.getShortName() + "!" + hexstr(relativeAddr);
        nameCache[addr] = desc;
    }
    else
    {
        std::string imgName = tmp.getShortName();
        PIN_LockClient();        
        std::string funcName = RTN_FindNameByAddress(addr);
        PIN_UnlockClient();
        if (funcName == ".text" || funcName == "unnamedImageEntryPoint" || (funcName[0] == '0' && funcName[1] == 'x'))
            funcName = hexstr(relativeAddr);
        if (funcName == "")
            funcName = hexstr(relativeAddr);
        desc = imgName + "!" + funcName;
        nameCache[addr] = desc;
#ifdef TARGET_IA32
        abi = ABI::ABI_STDCALL;
#elif TARGET_IA32e
        abi = ABI::ABI_FASTCALL;
#endif
        analysisFunction();        
    }

}

void Call::analysisFunction()
{
    PIN_LockClient();
    RTN rtn = RTN_CreateAt(addr, hexstr(addr));
    PIN_UnlockClient();

    if (RTN_Valid(rtn))
    {
        RTN_Open(rtn);
        for (INS ins = RTN_InsHead(rtn); INS_Valid(ins) && !INS_IsControlFlow(ins); ins = INS_Next(ins))
        {
            if (!INS_Valid(ins))
            {
                abi = ABI::ABI_ERROR;
                break;
            }
            if (INS_OperandCount(ins) >= 2)
            {
#ifdef TARGET_IA32
                //ecx 레지스터가 변경되면 thiscall 은 아니다.
                if (INS_OperandIsReg(ins, 0) && INS_OperandReg(ins, 0) == REG_ECX && INS_OperandWritten(ins, 0))
                    break;
                //ecx 에서 읽어서 다른 레지스터 또는 메모리로 값을 가지고 오는 경우
                else if (INS_IsMov(ins) && INS_OperandIsReg(ins, 1) && INS_OperandReg(ins, 1) == REG_ECX && INS_OperandReadOnly(ins, 1))
                {
                    abi = ABI::ABI_THISCALL;
                    abiCache[addr] = abi;
                    RTN_Close(rtn);
                    return;
                }
                else if (INS_IsSub(ins) && INS_OperandReg(ins, 0) == REG_ESP)
                {
                    noStackFrame = false;
                    stackCache[addr] = false;
                }
#else
                if (INS_IsSub(ins) && INS_OperandReg(ins, 0) == REG_RSP)
                {
                    noStackFrame = false;
                    stackCache[addr] = false;
                }
#endif
            }
        }
        RTN_Close(rtn);
    }
    else
    {
        //오류가 발생했을 시에도 분석이 완료된 것으로 가정한다. 
        abi = ABI::ABI_ERROR;
    }
    abiCache[addr] = abi;
}

void Call::descript()
{
    if (isCall())
    {
        std::cout << std::hex << "\nCall : " << ABISTR[abi] << " " << addr << " (" << desc << ")\n";
        if (parent != NULL)
            std::cout << std::hex << "Parent : " << parent->addr << " (" << parent->getDescription() << ")\n";
        else
        {
            //Root call 인 경우
            for (int i = 0; i < 3; i++)
                std::cout << "args[" << i << "] : " << hexstr(args[i]) << "\n";
        }
    }
    else if (isBranch())
        std::cout << "Branch : " << std::hex << addr << " (" << desc << ")\n";;
    for (auto const& child : childs)
        child->descript();
}


void Call::extractIndirect(std::stringstream& ss)
{
    if (isIndirectCall())
    {
        Image image = Image::getImageByAddress(callSite);
        ADDRINT relativeCallSite = callSite - image.getBaseAddress();
        ss << image.getShortName() << "!" << std::hex << relativeCallSite << " " << Image::getImageByAddress(addr).getShortName() << "!" << hexstr(relativeAddr) << "\n";
    }
    for (auto const& child : childs)
        child->extractIndirect(ss);
}

Call::~Call()
{
    addr = NULL;
    for (auto const& child : childs)
        delete child;
}
bool Call::isSystemCall() const
{
    return systemCall;
}
void Call::insertChild(Call* const call)
{
    childs.push_back(call);
}

bool hasSymbol(std::string name)
{
    const char* invalid[] = { "loc_", "sub_", "locret_", "SEH_" };
    for (auto const& n : invalid)
    {
        if (name.find(n) != std::string::npos)
            return false;
    }
    return true;
}

//IDA PRO integration
void Call::loadExternalInformation(const std::string imageName)
{
    const std::string mapPath = System::getSystemPath() + "\\" + imageName + ".map";
    const std::string cPath = System::getSystemPath() + "\\" + imageName + ".c";
    std::ifstream map(mapPath);
    char line[1024];
    if (!map.is_open())
        return;
    while (map.getline(line, sizeof(line)))
    {
        std::string tmp(line);
        if (tmp.find("Publics by Value") != std::string::npos)
        {
            map.getline(line, sizeof(line));
            break;
        }
    }
    while (map.getline(line, sizeof(line)))
    {
        std::string tmp(line);
        if (tmp.find("0001:") == std::string::npos)
            break;
        std::string name = tmp.substr(sizeof(size_t) * 3 + 9);
        if (hasSymbol(name))
        {
            ADDRINT addr = strtoull(tmp.substr(sizeof(size_t) + 2, sizeof(size_t) * 2).c_str(), NULL, 16) + 0x1000;
            if (name.find("(") != std::string::npos)
                name = name.substr(0, name.find("("));
            nameCache[addr + Image::getImageByName(imageName).getBaseAddress()] = name;
        }
    }
    map.close();
    std::ifstream src(cPath);
    if (!src.is_open())
    {
        nameCache.clear();
        return;
    }
    bool flag;
    while (src.getline(line, sizeof(line)))
    {
        std::string tmp(line);
        if (tmp.find("{") != std::string::npos)
            break;
        for (const auto& pair : nameCache)
        {
            flag = false;
            if (tmp.find(pair.second) != std::string::npos)
            {
                for (int i = 0; i < 5; i++)
                {
                    if (tmp.find(ABISTR[i]) != std::string::npos)
                    {
                        abiCache[pair.first + Image::getImageByName(imageName).getBaseAddress()] = static_cast<Call::ABI>(i);
                        flag = true;
                        break;
                    }
                }
            }
            if (flag)
                break;
        }
    }
}
