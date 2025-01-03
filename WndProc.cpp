#include "pin.H"
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include "Image.h"
#include "System.h"

using namespace std;

size_t targetCommand = WM_COMMAND;

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
unordered_map<ADDRINT, std::string> funcCache;

VOID checkArgs(ADDRINT target, BOOL taken, ADDRINT arg0, ADDRINT arg1, ADDRINT arg2, ADDRINT addr)
{
    if (!taken)
        return;
    targetCommand = atoi(System::getCustomValue("cmd").c_str());
    if (arg1 == targetCommand)
    {
        string targetFuncName;
        string insFuncName;
        if (funcCache.find(target) != funcCache.end())
            targetFuncName = funcCache[target];
        else
        {
            PIN_LockClient(); ;
            RTN_FindNameByAddress(target);
            PIN_UnlockClient();
        }
        if (funcCache.find(addr) != funcCache.end())
            targetFuncName = funcCache[addr];
        else
        {
            PIN_LockClient(); ;
            RTN_FindNameByAddress(addr);
            PIN_UnlockClient();
        }
        if (insMap.find(target + addr) != insMap.end())
            insMap[target + addr].count++;
        else
            insMap[target + addr] = { addr, target, 0, Image::getImageByAddress(addr).getShortName() + "!" + insFuncName,Image::getImageByAddress(target).getShortName() + "!" + targetFuncName };
    }
}

VOID fini(INT32 code, VOID* v)
{    
    cout << setw(45) << "Address" << setw(36) << "Disassemble" << setw(45) << "Target Address" << setw(10) << "Count" << endl;
    for (const auto& elem : insMap)
    {
        if (!Image::getImageByAddress(elem.second.target).isSystemImage())
            cout << hex << setw(45)  << hexstr(elem.second.addr) + " (" + elem.second.addrDesc + ")" << setw(36) << disassMap[elem.second.addr] << setw(45) << hexstr(elem.second.target) + " (" + elem.second.targetDesc + ")" << setw(10) << elem.second.count + 1 << endl;
    }
    outputFile.close();
}

VOID instrument(TRACE trace, VOID* v)
{
    string imgName = Image::getImageByAddress(TRACE_Address(trace)).getShortName();
    if (imgName != "user32.dll" && Image::getImageByAddress(TRACE_Address(trace)).isSystemImage())
        return;
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        INS ins = BBL_InsTail(bbl);
        if (!INS_Valid(ins))
        {
            cout << "Invalid instruction at " << hexstr(INS_Address(ins)) << endl;
            continue;
        }
        
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
}

VOID imgLoad(IMG img, VOID *v) 
{
    Image image(IMG_Id(img));      
    Image::writeMoudleInfo("module.txt");
}


int main(int argc, char* argv[])
{
    PIN_InitSymbols();
    PIN_Init(argc, argv);
    outputFile.open("wndproc.txt");
    if (!outputFile.is_open())
    {
        exit(-1);
    }
    cout.rdbuf(outputFile.rdbuf()); 
    TRACE_AddInstrumentFunction(instrument, 0);
    IMG_AddInstrumentFunction(imgLoad, 0);    
    PIN_AddFiniFunction(fini, 0);
    System::Init();
    PIN_StartProgram();
    return 0;
}