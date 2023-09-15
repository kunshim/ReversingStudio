#pragma once
#include "pin.H"
#include <iostream>
#include <_unordered_map.h>
#include <sstream>
#include <list>
#include "Args.h"
#include "System.h"

class Call
{
public:
    enum ABI
    {
        ABI_THISCALL = 0,
        ABI_STDCALL,
        ABI_FASTCALL,
        ABI_CDECL,
        ABI_USERCALL,
        ABI_ERROR,
    } abi;
    Call(ADDRINT addr, ADDRINT sfp, ADDRINT callSite, Call* parent, bool indirect);
    void setEnd()
    {
        endFlag = true;
    }
    bool isRunning() const
    {
        return !endFlag;
    }
    //If target is call, return true
    bool isCall() const
    {
        return sfp != 0;
    }
    // If target is branch, return true
    bool isBranch() const
    {
        return sfp == 0;
    }
    // If call is system call, return true
    bool isSystemCall() const
    {
        return systemCall;
    }
    // If call is indirect call, return true
    bool isIndirectCall() const
    {
        return indirect;
    }
    // Return function start address
    ADDRINT getAddress() const
    {
        return addr;
    }
    // Return base pointer (EBP, RBP)
    ADDRINT getFramePointer() const
    {
        return sfp;
    }
    // Return the address where call instruction execute
    ADDRINT getCallSite() const
    {
        return callSite;
    }
    // Return relative address
    ADDRINT getRelativeAddress() const
    {
        return relativeAddr;
    }
    // Return parent call
    Call* getParent() const
    {
        return parent;
    }
    const std::string& getDescription() const
    {
        return desc;
    }
    ABI getAbi() const
    {
        return abi;
    }
    void insertChild(Call* const call);
    void extractIndirect(std::stringstream& ss);
    int count = 1; 
    size_t args[3];
    void descript();
    static void loadExternalInformation(std::string imageName);
    ~Call();
private:
    static std::tr1::unordered_map<ADDRINT, std::string> nameCache;
    static std::tr1::unordered_map<ADDRINT, Call::ABI> abiCache;
    static std::tr1::unordered_map<ADDRINT, bool> stackCache;    
    std::list<Call*> childs;
    Call* parent;
    std::string desc;
    bool endFlag;
    bool systemCall;
    bool indirect;
    bool noStackFrame;
    void analysisFunction();
    static const char* ABISTR[6];
    ADDRINT sfp;
    ADDRINT addr;
    ADDRINT callSite;
    ADDRINT relativeAddr;
};
