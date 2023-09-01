#pragma once
#include "pin.H"
#include <iostream>
#include <vector>
#include <string>
#include <_unordered_map.h>
#include <memory>
#include "Image.h"
#include "Call.h"

class CallTrack 
{ 
private:
    std::tr1::unordered_map<ADDRINT, Call*> callCache;
    int sequence = 0;
public:
    Call* root;
    size_t hash;
    Call* currentCall;
    CallTrack(ADDRINT addr, ADDRINT sfp);
    void insertChild(Call* call);
    void insertBranch(Call* call);
    void descript();
    void analyze();
    ~CallTrack();
    bool operator==(const CallTrack& other) const;
};
