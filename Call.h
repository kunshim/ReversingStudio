#pragma once
#include "pin.H"
#include <iostream>
#include <_unordered_map.h>
#include <sstream>
#include <list>
class Call
{
private:
    enum ABI
    {
        ABI_THISCALL = 0,
        ABI_STDCALL,
        ABI_FASTCALL
    } abi;
    static std::tr1::unordered_map<ADDRINT, std::string> nameCache;
    static std::tr1::unordered_map<ADDRINT, Call::ABI> abiCache;
    bool endFlag;
    void analysisABI();
    static const char* ABISTR[3];
public:
    void setEnd();
    bool isEnd();
    bool isSystemCall;
    int count = 1; 
    ADDRINT addr;
    ADDRINT sfp; //saved frame pointer
    std::string desc;
    std::list<Call*> branches;
    std::list<Call*> childs;
    Call* parent; //상위 콜
    Call(ADDRINT addr, ADDRINT sfp);
    void descript();
    ~Call();
};

