#include "Calltrack.h"
#include "System.h"

CallTrack::CallTrack(Call* root) : root(root)
{
    currentCall = root;
    hash = root->getRelativeAddress();
}

//자식콜 저장
void CallTrack::insertChildCall(Call* const call)
{
    //시스템 콜의 내부에서 발생하는 콜들을 제외하고 해시를 계산한다. 해시값 계산은 상대주소로 계산한다.
    //기본적으로 시스템 콜 내부는 계측을 안하게 돼있음.
    if(!call->getParent()->isSystemCall())
        hash ^= (call->getRelativeAddress() + sequence++);
    currentCall->insertChild(const_cast<Call*>(call));
    if (!call->getParent()->isSystemCall() || System::getCustomValue("track-system") == "true")
    {
        //call 들의 개수를 파악하기 위해서 캐시를 활용한다.
        if (callCache.find(call->getAddress()) == callCache.end())
            callCache[call->getAddress()] = const_cast<Call*>(call);
        else
            callCache[call->getAddress()]->count++;
    }
}

void CallTrack::insertBranch(const Call* const call)
{  
    //브랜치의 경우에는 부모가 없기 때문에 본인이 시스템콜인지 확인해야 한다.
    if (!call->isSystemCall() || System::getCustomValue("track-system") == "true")
        hash ^= (call->getRelativeAddress() + sequence++);
    currentCall->insertChild(const_cast<Call*>(call));
}

//콜트래킹 결과 출력
void CallTrack::descript() const
{
    std::cout << "Analysis of " << std::hex << root->getAddress() << "\n";
    std::cout << "Hash : " << std::hex << hash << "\n";
    root->descript();
    std::cout << "End analysis of " << std::hex << root->getAddress() << "\n" << std::endl;
    printInnerCallCount();
}

//함수 콜 카운팅 결과 출력력
void CallTrack::printInnerCallCount() const
{
    if (!callCache.empty())
    {
        std::cout << std::setw(55) << "Address" << std::setw(10) << "Count\n";
        for(auto const& elem : callCache)
        {
            auto call = elem.second;
            std::cout << std::setw(55) << call->getDescription() << std::setw(10) << call->count << "\n";  
        }
        std::cout << std::endl;
    }
}

//커버리지 파일 저장
void CallTrack::saveCoverage() const
{
    std::string pathPrefix = "./";
    std::ofstream covFile;
    std::stringstream filename;
    filename << std::hex << pathPrefix << "cov_" << root->getAddress() << "_" << hash  << ".txt";
    covFile.open((filename.str()).c_str());
    if (!covFile.is_open())
        return;
    for (auto const& addr : bbls)
    {
        Image img = Image::getImageByAddress(addr);
        ADDRINT relativeAddr = addr - Image::getImageByAddress(addr).getBaseAddress();
        std::string desc = img.getShortName() + "+" + hexstr(relativeAddr);
        covFile << desc << "\n";
    }
    covFile.close();
}

//IDA 분석용 간접호출 파일 저장
void CallTrack::saveIndirect() const
{
    std::string pathPrefix = "./";
    std::ofstream indirectFile;
    std::stringstream filename, ss;
    filename << std::hex << pathPrefix << "ind_" << root->getAddress() << "_" << hash << ".txt";
    indirectFile.open(filename.str().c_str());
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
