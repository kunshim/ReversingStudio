#pragma once
#include "pin.H"
#include <iostream>
#include <string>
namespace WINDOWS
{
    #include <Windows.h>
};
constexpr int CP_949 = 949;
constexpr int SHIFT_JIS = 932;
constexpr int UTF16 = 1200;
constexpr int UTF8 = 65001;

class Args
{
private:
    ADDRINT arg;
    enum TYPE
    {
        ARG_TYPE_CHAR,
        ARG_TYPE_WCHAR,
        ARG_TYPE_PTRCHAIN,
        ARG_TYPE_INT
    } type;
public:
    Args(ADDRINT arg);
    void descript();
};