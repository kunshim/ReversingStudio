#include "Calltrack.h"

CallTrack::CallTrack(ADDRINT addr, ADDRINT sfp)
{
    root = new Call(addr, sfp);
    currentCall = root;
    hash = addr - Image::getImageByAddress(addr).getBaseAddress();
}

//자식콜 저장
void CallTrack::insertChild(const Call* call)
{
    //시스템 콜의 내부에서 발생하는 콜들을 제외하고 해시를 계산한다. 해시값 계산은 상대주소로 계산한다.
    //기본적으로 시스템 콜 내부는 계측을 안하게 돼있음.
    if(!call->parent->isSystemCall)
        hash ^= ((call->addr - Image::getImageByAddress(call->addr).getBaseAddress()) + sequence++);
    currentCall->childs.push_back(call);
    if(!call->parent->isSystemCall)
    {
        //call 들의 개수를 파악하기 위해서 캐시를 활용한다.
        if (callCache.find(call->addr) == callCache.end())
            callCache[call->addr] = call;
        else
            callCache[call->addr]->count++;
    }
}

void CallTrack::insertBranch(const Call* call)
{  
    //브랜치의 경우에는 부모가 없기 때문에 본인이 시스템콜인지 확인해야 한다.
    if (!call->isSystemCall)
        hash ^= ((call->addr - Image::getImageByAddress(call->addr).getBaseAddress()) + sequence++);
    currentCall->childs.push_back(call);
}

//콜트래킹 결과 출력
void CallTrack::descript()
{
    std::cout << "Analysis of " << std::hex << root->addr << "\n";
    std::cout << "Hash : " << std::hex << hash << "\n";
    root->descript();
    std::cout << "End analysis of " << std::hex << root->addr << std::endl;
    analyze();
}

//함수 콜 카운팅 결과 출력력
void CallTrack::printInnerCallCount()
{
    if (!callCache.empty())
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

//커버리지 파일 저장
void CallTrack::saveCoverage()
{
    std::string pathPrefix = "./";
    std::ofstream covFile;
    std::stringstream filename, ss;
    filename << pathPrefix << "cov_" << std::hex << root->addr << "_" << hash << ".txt";
    covFile.open((filename.str()).c_str());
    if (!covFile.is_open())
        return;
    root->extract(ss);
    covFile << ss.str() << endl;
    covFile.close();
}

//IDA 분석용 간접호출 파일 저장
void CallTrack::saveIndirect()
{
    std::string pathPrefix = "./";
    std::ofstream indirectFile;
    std::stringstream filename, ss;
    filename << pathPrefix << "ind_" << std::hex << root->addr << "_" << hash << ".txt";
    indirectFile.open(filename.str()).c_str();
    if (!indirectFile.is_open())
        return;
    root->extractIndirect(ss);
    indirectFile << ss.str() << std::endl;
    indirectFile.close();
}

CallTrack::~CallTrack()
{
    //재귀적으로 하위 콜도 다 정리됨
    delete root;
}

bool CallTrack::operator==(const CallTrack& other) const 
{
    return this->hash == other.hash;
}

Call* CallTrack::getRootCall()
{
    return root;
}

Call* CallTrack::getCurrentCall()
{
    return currentCall;
}