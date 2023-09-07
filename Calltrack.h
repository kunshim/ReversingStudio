#pragma once
#include "pin.H"
#include <iostream>
#include <vector>
#include <string>
#include <_unordered_map.h>
#include <memory>
#include "Image.h"
#include "Call.h"
#include "fstream"

class CallTrack 
{ 
private:
    void printInnerCallCount();
    std::tr1::unordered_map<ADDRINT, Call*> callCache;
    size_t sequence = 0;
    size_t hash;
    Call* root;
    Call* currentCall;
public:
    CallTrack(ADDRINT addr, ADDRINT sfp);
    Call* getRootCall();
    Call* getCurrentCall();
    void insertChild(const Call* call);
    void insertBranch(const Call* call);
    void saveCoverage();
    void saveIndirect();
    void descript();
    ~CallTrack();
    bool operator==(const CallTrack& other) const;
};
