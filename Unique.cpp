#include "pin.H"
#include <iostream>
#include <fstream>
#include <string>
#include <_unordered_map.h>
#include <sstream>
#include <fstream>
#include "Calltrack.h"
using namespace std;
using namespace tr1;

struct procArgs
{
    UINT uMsg;
    UINT wParam;
    UINT lParam;
    bool operator==(const procArgs& other) const {
        return uMsg == other.uMsg && wParam == other.wParam && lParam == other.lParam;
    }
}typedef procArgs;

ofstream outputFile; //출력될 파일
ofstream moduleInfoFile; //모듈정보가 기록되는 파일
vector<ADDRINT> wndproc_list;
vector<CallTrack*> callTrackList;
vector<procArgs> argsList;
unordered_map<ADDRINT, string>insMap;
//현재 진행중인 콜스택의 인자 
UINT cuMsg, cwParam, clParam;
//현재 콜트래킹중인 객체
CallTrack* currentCallTrack;
OS_THREAD_ID mainTid;
bool started = false;

VOID fini(INT32 code, VOID* v)
{
    Image::writeMoudleInfo("module.txt");
    cout << endl;
    outputFile.close();
}

VOID trackBranch(ADDRINT addr, ADDRINT target, BOOL taken, ADDRINT arg0, ADDRINT arg1, ADDRINT arg2)
{
    //브랜치가 실제로 실행된 경우만 실행
    if (!taken)
        return;  
    if (PIN_GetTid() != mainTid)
        return;
    //지정한 WndProc 내부에서 발생하는 이벤트만 기록한다
    if (currentCallTrack == NULL || currentCallTrack->root->isEnd())
        return;    
    //만약 WndProc 함수 내부에서 별도의 스레드를 생성한 경우에 해당 루틴은 무시한다.  
    Call* newCall = new Call(target, 0);
    currentCallTrack->insertBranch(newCall);
}

VOID trackCall(ADDRINT addr, ADDRINT target, ADDRINT arg0, ADDRINT arg1, ADDRINT arg2, ADDRINT sfp)
{
    if (PIN_GetTid() != mainTid)
        return;  
    if (currentCallTrack == NULL || currentCallTrack->root->isEnd())
        return;      
    Call* newCall = new Call(target, sfp);
    newCall->parent = currentCallTrack->currentCall;
    currentCallTrack->insertChild(newCall);
    currentCallTrack->currentCall = newCall;
}

VOID trackRet(ADDRINT sfp)
{ 
    if (PIN_GetTid() != mainTid)
        return;
    if (currentCallTrack == NULL)
        return;
    if (currentCallTrack->root->sfp == sfp)
    {
        currentCallTrack->root->setEnd();
        bool isDuplicated = false;
        for (int i = 0; i < callTrackList.size(); i++)
        {
            if (callTrackList[i]->hash == currentCallTrack->hash)
            {
                isDuplicated = true;
                break;
            }
        }
        cout << "\n" << hex << cuMsg << " " << cwParam << " " << clParam << endl;
        if (!isDuplicated)
        {
            currentCallTrack->descript();
            currentCallTrack->analyze();
            callTrackList.push_back(currentCallTrack);
            argsList.push_back({cuMsg, cwParam, clParam});
        }
        else
        {
            delete currentCallTrack;
        }
        currentCallTrack = NULL;
    }
    else if (currentCallTrack->currentCall->sfp == sfp)
    {
        currentCallTrack->currentCall->setEnd();
        currentCallTrack->currentCall = currentCallTrack->currentCall->parent;
    }
}
#ifdef TARGET_IA32
VOID hookEntry(ADDRINT addr, ADDRINT sfp, ADDRINT sp)
{
    if (!currentCallTrack)
    {
        started = true;
        cuMsg = *(ADDRINT*)(sp + 8);
        cwParam = *(ADDRINT*)(sp + 12);
        clParam = *(ADDRINT*)(sp + 16);
        mainTid = PIN_GetTid();
        currentCallTrack = new CallTrack(addr, sfp);
    }
}
#elif TARGET_IA32E
VOID hookEntry(ADDRINT addr, ADDRINT sfp, ADDRINT rcx, ADDRINT rdx, ADDRINT r8, ADDRINT r9)
{
    if (!currentCallTrack)
    {
        cuMsg = rdx;
        cwParam = r8;
        clParam = r9;
        mainTid = PIN_GetTid();
        currentCallTrack = new CallTrack(addr, sfp);
    }
}
#endif

VOID init(VOID* v)
{
    currentCallTrack = NULL;
    for (const auto& addr : wndproc_list)
    {
        PIN_LockClient();
        stringstream ss;
        ss << hex << addr;
        RTN rtn = RTN_CreateAt(addr, ss.str());
        PIN_UnlockClient();      
        ASSERT(RTN_Valid(rtn), "Invalid RTN");
        RTN_Open(rtn);
        INS head = RTN_InsHead(rtn);
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

VOID instrumentIns(INS ins, VOID* v)
{
    //속도를 위해서 System Image는 Instrument 를 하지 않는다. JIT 방식이라서 금방 빨라진다. 
    //처음에 이미지 로딩할 때 Instrument 하는게 더 나을 수도..
    //!Image::getImageByAddress(INS_Address(ins)).isSystemImage() && 
    if (INS_IsControlFlow(ins))
    {
        insMap[INS_Address(ins)] = INS_Disassemble(ins);
        if (INS_IsRet(ins))
        {
           INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trackRet), 
           IARG_REG_VALUE, REG_GBP,
           IARG_END);
        }
        else if (!Image::getImageByAddress(INS_Address(ins)).isSystemImage() && INS_IsDirectControlFlow(ins))
        {
            if (INS_IsCall(ins))
            {
                INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trackCall),
                    IARG_INST_PTR,
                    IARG_ADDRINT, INS_DirectBranchOrCallTargetAddress(ins),
                    IARG_FUNCARG_CALLSITE_VALUE, 0,
                    IARG_FUNCARG_CALLSITE_VALUE, 1,
                    IARG_FUNCARG_CALLSITE_VALUE, 2,
                    IARG_REG_VALUE, REG_GBP,
                    IARG_END);
            }
            else if (INS_IsBranch(ins))
            {
                INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(trackBranch),
                    IARG_INST_PTR,
                    IARG_ADDRINT, INS_DirectBranchOrCallTargetAddress(ins), IARG_BRANCH_TAKEN,
                    IARG_FUNCARG_CALLSITE_VALUE, 0,
                    IARG_FUNCARG_CALLSITE_VALUE, 1,
                    IARG_FUNCARG_CALLSITE_VALUE, 2,
                    IARG_END);
            }
        }
        else if (!Image::getImageByAddress(INS_Address(ins)).isSystemImage() && INS_IsIndirectControlFlow(ins))
        {
            if (INS_IsBranch(ins))
            {
                INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(trackBranch),
                    IARG_INST_PTR,
                    IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN,
                    IARG_FUNCARG_CALLSITE_VALUE, 0,
                    IARG_FUNCARG_CALLSITE_VALUE, 1,
                    IARG_FUNCARG_CALLSITE_VALUE, 2,
                    IARG_END);
            }
            else if (INS_IsCall(ins))
            {
                INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(trackCall),
                    IARG_INST_PTR,
                    IARG_BRANCH_TARGET_ADDR,
                    IARG_FUNCARG_CALLSITE_VALUE, 0,
                    IARG_FUNCARG_CALLSITE_VALUE, 1,
                    IARG_FUNCARG_CALLSITE_VALUE, 2,
                    IARG_REG_VALUE, REG_GBP,
                    IARG_END);
            }
        }
    }
}

VOID imgLoad(IMG img, VOID *v) 
{
    Image image(IMG_Id(img));    
    string imageName = image.getShortName();
    ADDRINT baseAddress = IMG_LowAddress(img);
    if (imageName.find("win32dialog.dll") != string::npos)
        wndproc_list.push_back(baseAddress + 0x67b0);
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
    PIN_AddApplicationStartFunction(init, 0);
    INS_AddInstrumentFunction(instrumentIns, 0);
    IMG_AddInstrumentFunction(imgLoad, 0);
    PIN_AddFiniFunction(fini, 0);
    PIN_StartProgram();
    return 0;
}