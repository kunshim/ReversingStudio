#include "pin.H"
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <memory>
#include "Calltrack.h"
#include "System.h"
namespace WINDOWS
{
#include <Windows.h>
}
using namespace std;

ofstream outputFile; 
ofstream moduleInfoFile; 
vector<CallTrack*> callTrackList; 
CallTrack* currentCallTrack; 
OS_THREAD_ID mainTid; 

VOID trackRet(ADDRINT addr, ADDRINT sfp);
VOID fini(INT32 code, VOID* v);
VOID trackBranch(ADDRINT addr, ADDRINT target, bool isIndirect, BOOL taken, ADDRINT arg0, ADDRINT arg1, ADDRINT arg2, ADDRINT next);
VOID trackCall(ADDRINT addr, ADDRINT target, bool isIndirect, ADDRINT arg0, ADDRINT arg1, ADDRINT arg2, ADDRINT sfp, ADDRINT ecx);

VOID fini(INT32 code, VOID* v)
{
    cout << endl;
    outputFile.close();
    Image::writeMoudleInfo("module.txt");
}

VOID trackBranch(ADDRINT addr, ADDRINT target, bool isIndirect, BOOL taken, ADDRINT arg0, ADDRINT arg1, ADDRINT arg2, ADDRINT next)
{
    if (PIN_GetTid() != mainTid)
        return;
    //지정한 WndProc 내부에서 발생하는 이벤트만 기록한다
    if (currentCallTrack == NULL || !currentCallTrack->getRootCall()->isRunning())
        return;    
    if (!taken)
        target = next;
    Call* newCall = new Call(target, NULL, addr, NULL, isIndirect);
    currentCallTrack->insertBranch(newCall);
    
}

VOID trackCall(ADDRINT addr, ADDRINT target, bool isIndirect, ADDRINT arg0, ADDRINT arg1, ADDRINT arg2, ADDRINT sfp, ADDRINT ecx)
{
    if (PIN_GetTid() != mainTid)
        return;
    if (currentCallTrack == NULL || !currentCallTrack->getRootCall()->isRunning())
        return;
    Call* newCall;
    newCall = new Call(target, sfp, addr, currentCallTrack->getCurrentCall(), isIndirect);
#ifdef TARGET_IA32
    if (newCall->abi == Call::ABI::ABI_THISCALL)
        newCall->args[0] = ecx;
    else
        newCall->args[0] = arg0;
    newCall->args[1] = arg1;
    newCall->args[2] = arg2;
#elif TARGET_IA32e
    
#endif
    currentCallTrack->insertChildCall(newCall);
    if (!newCall->isSystemCall() || System::getCustomValue("track-system") == "true")
        currentCallTrack->setCurrentCall(newCall);
}
    

VOID trackRet(ADDRINT addr, ADDRINT sfp)
{ 
    if (PIN_GetTid() != mainTid)
        return;
    if (currentCallTrack == NULL)
        return;
    if (!currentCallTrack->getCurrentCall()->getParent())
    {
        currentCallTrack->getRootCall()->setEnd();
        bool isDuplicated = false;
        for (int i = 0; i < callTrackList.size(); i++)
        {
            if (callTrackList[i]->eqaul(currentCallTrack))
            {
                isDuplicated = true;
                break;
            }
        }
        if (!isDuplicated)
        { 
            currentCallTrack->descript();
            if (System::getCustomValue("unique") != "false")
                callTrackList.push_back(currentCallTrack);
            if (System::getCustomValue("cov") == "true")
                currentCallTrack->saveCoverage();
            if (System::getCustomValue("indirect") == "true")
                currentCallTrack->saveIndirect();
        }
        else
            delete currentCallTrack;
        currentCallTrack = NULL;
    }
    else
    {
        currentCallTrack->getCurrentCall()->setEnd();
        currentCallTrack->setCurrentCall(currentCallTrack->getCurrentCall()->getParent());
    }
}
#ifdef TARGET_IA32
VOID hookEntry(ADDRINT addr, ADDRINT sfp, ADDRINT sp)
{
    if (!currentCallTrack)
    {      
        mainTid = PIN_GetTid();
        //루트 콜의 경우 frame pointer 가 NULL이 아니면서 부모가 NULL이다. -> 콜중에서 유일하게 부모가 NULL 이다.
        Call* root = new Call(addr, sfp, NULL, NULL, false);
        currentCallTrack = new CallTrack(root);
        currentCallTrack->getRootCall()->args[0] = *(ADDRINT*)(sp + 4);
        currentCallTrack->getRootCall()->args[1] = *(ADDRINT*)(sp + 8);
        currentCallTrack->getRootCall()->args[2] = *(ADDRINT*)(sp + 12);
    }
}
#elif TARGET_IA32E
VOID hookEntry(ADDRINT addr, ADDRINT sfp, ADDRINT rcx, ADDRINT rdx, ADDRINT r8, ADDRINT r9)
{
    if (!currentCallTrack)
    {
        mainTid = PIN_GetTid();
        Call* root = new Call(addr, sfp, NULL, NULL, false);
        currentCallTrack = new CallTrack(root);
        currentCallTrack->getRootCall()->args[0] = rcx;
        currentCallTrack->getRootCall()->args[1] = rdx;
        currentCallTrack->getRootCall()->args[2] = r8;
    }
}
#endif

VOID init(VOID* v)
{
    currentCallTrack = NULL;    
}

VOID imgLoad(IMG img, VOID *v) 
{
    Image image(IMG_Id(img));    
    string imageName = image.getShortName();
    Call::loadExternalInformation(imageName);
    for (int i = 0; i < System::getCustomArgsCount(); i++)
    {
        string arg = System::getCustomArgs(i);
        string libname = arg.substr(0, arg.find('+'));
        string addrString = arg.substr(arg.find('+') + 1);
        if (libname.find(imageName) != string::npos)
        {            
            if (addrString.find("0x") != string::npos)
                addrString = addrString.substr(2);
            ADDRINT addr = image.getBaseAddress() + strtoull(addrString.c_str(), NULL, 16);
            PIN_LockClient();
            RTN rtn = RTN_CreateAt(addr, hexstr(addr));          
            PIN_UnlockClient();           
            if (!RTN_Valid(rtn))
            {
                cout << "Invalid RTN at " << hexstr(addr) << endl;
                outputFile.close();
                exit(-1);
            }
            RTN_Open(rtn);
            INS head = RTN_InsHead(rtn);
            if (!INS_Valid(head))
            {
                cout << "Invalid INS at " << hexstr(addr) << endl;
                outputFile.close();
                exit(-1);
            }
#ifdef TARGET_IA32
            INS_InsertCall(head, IPOINT_BEFORE, AFUNPTR(hookEntry),
                IARG_ADDRINT, addr,
                IARG_REG_VALUE, REG_GBP,
                IARG_REG_VALUE, REG_STACK_PTR,
                IARG_END);
#elif TARGET_IA32E
            INS_InsertCall(head, IPOINT_BEFORE, AFUNPTR(hookEntry),
                IARG_ADDRINT, addr,
                IARG_REG_VALUE, REG_GBP,
                IARG_REG_VALUE, REG_RCX,
                IARG_REG_VALUE, REG_RDX,
                IARG_REG_VALUE, REG_R8,
                IARG_REG_VALUE, REG_R9,
                IARG_END);
#endif
            RTN_Close(rtn);
        }
    }
    Image::writeMoudleInfo("module.txt");
}

VOID insertBBInCalltrack(ADDRINT addr)
{
    if (PIN_GetTid() != mainTid || currentCallTrack == NULL)
        return;
    currentCallTrack->insertBBL(addr);
}

VOID trace(TRACE trace, VOID* v)
{
    if (Image::getImageByAddress(TRACE_Address(trace)).isSystemImage() && System::getCustomValue("track-system") != "true")
        return;
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {            
        if (System::getCustomValue("cov") == "true")
        {
            for (INS ins = BBL_InsHead(bbl); INS_Valid(ins) && (INS_Address(ins) <= INS_Address(BBL_InsTail(bbl))); ins = INS_Next(ins))
                BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)insertBBInCalltrack, IARG_ADDRINT, INS_Address(ins), IARG_END);
        }
        INS ins = BBL_InsTail(bbl);
        if (!INS_Valid(ins))
        {
            cout << "Invalid instruction at " << hexstr(INS_Address(ins)) << endl;
            continue;
        }
        if (INS_IsControlFlow(ins))
        {
            if (INS_IsRet(ins))
            {
                INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trackRet),
                    IARG_INST_PTR,
                    IARG_REG_VALUE, REG_GBP,
                    IARG_END);

            }
            if (INS_IsDirectControlFlow(ins))
            {
                if (INS_IsCall(ins))
                {
                    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trackCall),
                        IARG_INST_PTR,
                        IARG_ADDRINT, INS_DirectControlFlowTargetAddress(ins),
                        IARG_BOOL, false,
                        IARG_FUNCARG_CALLSITE_VALUE, 0,
                        IARG_FUNCARG_CALLSITE_VALUE, 1,
                        IARG_FUNCARG_CALLSITE_VALUE, 2,
                        IARG_REG_VALUE, REG_GBP,
                        IARG_REG_VALUE, REG_ECX,
                        IARG_END);
                }
                else if (INS_IsBranch(ins))
                {
                    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trackBranch),
                        IARG_INST_PTR,
                        IARG_ADDRINT, INS_DirectControlFlowTargetAddress(ins), IARG_BRANCH_TAKEN,
                        IARG_BOOL, false,
                        IARG_FUNCARG_CALLSITE_VALUE, 0,
                        IARG_FUNCARG_CALLSITE_VALUE, 1,
                        IARG_FUNCARG_CALLSITE_VALUE, 2,
                        IARG_ADDRINT, INS_NextAddress(ins),
                        IARG_END);
                }
            }
            else if (INS_IsIndirectControlFlow(ins))
            {
                if (INS_IsBranch(ins))
                {
                    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trackBranch),
                        IARG_INST_PTR,
                        IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, 
                        IARG_BOOL, true,
                        IARG_FUNCARG_CALLSITE_VALUE, 0,
                        IARG_FUNCARG_CALLSITE_VALUE, 1,
                        IARG_FUNCARG_CALLSITE_VALUE, 2,
                        IARG_ADDRINT, INS_NextAddress(ins),
                        IARG_END);
                }
                else if (INS_IsCall(ins))
                {
                    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trackCall),
                        IARG_INST_PTR,
                        IARG_BRANCH_TARGET_ADDR,
                        IARG_BOOL, true,
                        IARG_FUNCARG_CALLSITE_VALUE, 0,
                        IARG_FUNCARG_CALLSITE_VALUE, 1,
                        IARG_FUNCARG_CALLSITE_VALUE, 2,
                        IARG_REG_VALUE, REG_GBP,
                        IARG_REG_VALUE, REG_ECX,
                        IARG_END);
                }
            }
        } 
    }
}

int main(int argc, char* argv[])
{
    PIN_InitSymbols();
    PIN_Init(argc, argv);
    outputFile.open("unique.txt");
    if (!outputFile.is_open())
        exit(-1);
    cout.rdbuf(outputFile.rdbuf()); 
    cerr.rdbuf(outputFile.rdbuf());
    cout.tie(0);
    ios_base::sync_with_stdio(false);
    PIN_AddApplicationStartFunction(init, 0);    
    TRACE_AddInstrumentFunction(trace, 0);
    IMG_AddInstrumentFunction(imgLoad, 0);
    PIN_AddFiniFunction(fini, 0);    
    PIN_StartProgram();
    return 0;
}