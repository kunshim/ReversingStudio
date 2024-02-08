#pragma once
#include "pin.H"
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "Image.h"
#include "Call.h"
#include "fstream"

class CallTrack 
{ 
private:
    void printInnerCallCount() const;
    std::unordered_map<ADDRINT, Call*> callCache;
    std::list<ADDRINT> bbls; //basic block list for create code coverage
    size_t sequence = 0;
    size_t hash;
    Call* const root;
    Call* currentCall;
public:
    CallTrack(Call* root);
    Call* getRootCall() const
    {
        return root;
    }
    Call* getCurrentCall() const
    {
        return currentCall;
    }
    void setCurrentCall(Call* currentCall)
    {
        this->currentCall = currentCall;
    }
    void insertChildCall(Call* const call);
    void insertBBL(ADDRINT addr)
    {
        bbls.push_back(addr);
    }
    void insertBranch(const Call* call);
    void saveCoverage() const;
    void saveIndirect() const;
    void descript() const;
    ~CallTrack();
    size_t getHash() const
    {
        return hash;
    }
    bool eqaul(const CallTrack* const other) const
    {
        return this->hash == other->getHash();
    }
};
