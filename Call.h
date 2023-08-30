#pragma once
#include "pin.H"
#include <iostream>
#include <_unordered_map.h>
#include <sstream>
#include <list>
class Call
{
private:
    static std::tr1::unordered_map<ADDRINT, std::string> nameCache;
    bool endFlag;
    void analysisABI();
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
    enum ABI
    {
        ABI_THISCALL,
        ABI_STDCALL,
        ABI_FASTCALL
    } abi;
};

