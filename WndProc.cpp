#include "pin.H"
#include <iostream>
#include <fstream>
#include <string>
#include <_unordered_map.h>
#include "Image.h"

using namespace std;
using namespace tr1;

#define WM_COMMAND 0x111

struct bunch
{
    size_t addr;
    size_t target;
    int count;
    string addrDesc;
    string targetDesc;
}typedef bunch;

ofstream outputFile;
unordered_map<ADDRINT, std::string> disassMap;
unordered_map<ADDRINT, bunch> insMap;
unordered_map<string, ADDRINT> imgMap;

VOID checkArgs(ADDRINT target, BOOL taken, ADDRINT arg0, ADDRINT arg1, ADDRINT arg2, ADDRINT addr)
{
    if (!taken)
        return;
    if (arg1 == WM_COMMAND)
    {
        PIN_LockClient();    ;
        string targetFuncName = RTN_FindNameByAddress(target);
        string insFuncName = RTN_FindNameByAddress(addr);
        PIN_UnlockClient();
        if (insMap.find(target + addr) != insMap.end())
            insMap[target + addr].count++;
        else
            insMap[target + addr] = { addr, target, 0, Image::getImageByAddress(addr).getShortName() + "!" + insFuncName,Image::getImageByAddress(target).getShortName() + "!" + targetFuncName };
    }
}

string int2HexStr(ADDRINT val)
{
    stringstream ss;
    ss << hex << val;
    return ss.str();
}

VOID fini(INT32 code, VOID* v)
{
    Image::writeMoudleInfo("module.txt");
    cout << setw(45) << "Address" << setw(36) << "Disassemble" << setw(45) << "Target Address" << setw(10) << "Count" << endl;
    for (const auto& elem : insMap)
    {
        if (!Image::getImageByAddress(elem.second.target).isSystemImage())
            cout << hex << setw(45)  << int2HexStr(elem.second.addr) + " (" + elem.second.addrDesc + ")" << setw(36) << disassMap[elem.second.addr] << setw(45) << int2HexStr(elem.second.target) + " (" + elem.second.targetDesc + ")" << setw(10) << elem.second.count << endl;
    }
    outputFile.close();
}

VOID instrument(INS ins, VOID* v)
{
    string imgName = Image::getImageByAddress(INS_Address(ins)).getShortName();
    if (imgName != "user32.dll" && Image::getImageByAddress(INS_Address(ins)).isSystemImage())
        return;
    if (INS_IsControlFlow(ins))
    {
        if (!INS_IsDirectControlFlow(ins))
        {
            if (INS_IsBranch(ins))
            {
                disassMap[INS_Address(ins)] = INS_Disassemble(ins);
                INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(checkArgs),
                    IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN,
                    IARG_FUNCARG_CALLSITE_VALUE, 0,
                    IARG_FUNCARG_CALLSITE_VALUE, 1,
                    IARG_FUNCARG_CALLSITE_VALUE, 2,
                    IARG_INST_PTR,
                    IARG_END);
            }
            else if (INS_IsCall(ins))
            {
                disassMap[INS_Address(ins)] = INS_Disassemble(ins);
                INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(checkArgs),
                    IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN,
                    IARG_FUNCARG_CALLSITE_VALUE, 0,
                    IARG_FUNCARG_CALLSITE_VALUE, 1,
                    IARG_FUNCARG_CALLSITE_VALUE, 2,
                    IARG_INST_PTR,
                    IARG_END);
            }
        }
    }
}

VOID imgLoad(IMG img, VOID *v) 
{
    Image image(IMG_Id(img));      
}


int main(int argc, char* argv[])
{
    PIN_InitSymbols();
    PIN_Init(argc, argv);
    outputFile.open("wndproc.txt");
    if (!outputFile.is_open())
        exit(-1);
    cout.rdbuf(outputFile.rdbuf()); 
    IMG_AddInstrumentFunction(imgLoad, 0);
    INS_AddInstrumentFunction(instrument, 0);
    PIN_AddFiniFunction(fini, 0);
    PIN_StartProgram();
    return 0;
}