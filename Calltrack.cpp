#include "Calltrack.h"

CallTrack::CallTrack(ADDRINT addr, ADDRINT sfp) : hash(addr)
{
    root = new Call(addr, sfp);
    currentCall = root;
}

void CallTrack::insertChild(Call* call)
{
    if (!call->isSystemCall)
        hash ^= (call->addr + sequence++);
    currentCall->childs.push_back(call);
    if(call->parent)
    {
        if (!call->parent->isSystemCall)
        {
            if (callCache.find(call->addr) == callCache.end())
                callCache[call->addr] = call;
            else
                callCache[call->addr]->count++;
        }
    }
}

void CallTrack::insertBranch(Call* call)
{
    if(!call->isSystemCall)
        hash ^= (call->addr + sequence++);
    currentCall->branches.push_back(call);
}

void CallTrack::analyze()
{
    if (callCache.size())
    {
        std::cout << std::setw(45) << "Address" << std::setw(10) << "Count\n";
        for(auto const& elem : callCache)
        {
            auto call = elem.second;
            std::cout << std::setw(45) << call->desc << std::setw(10) << call->count << "\n";  
        }
        std::cout << std::endl;
    }
}

CallTrack::~CallTrack()
{
    delete root;
    callCache.clear();
}

bool CallTrack::operator==(const CallTrack& other) const 
{
    return this->hash == other.hash;
}