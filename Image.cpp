#include "Image.h"


std::vector<Image> Image::imageList;
Image Image::invalid(-1);
size_t count = 0;

std::string Image::makeShortImgName(std::string name)
{
    size_t lastBackslashPos = name.find_last_of('\\');
    if (lastBackslashPos != std::string::npos) 
        return name.substr(++lastBackslashPos, name.length());     
    else
        return "";
}

std::string toHexStr(std::string target)
{
    for (int i = 0; i < target.length(); i++)
    {
        if (target[i] > '9' || target[i] < '0')
            return target;
    }
    std::stringstream hexStream;
    hexStream << std::hex << std::atoi(target.c_str());
    return hexStream.str();
}

Image::Image(UINT32 id) 
{
    IMG img = IMG_FindImgById(id);
    if (IMG_Valid(img))
    {
        this->id = id;
        fullName = IMG_Name(img);
        shortName = makeShortImgName(fullName);
        if (fullName.find("C:\\Windows") != std::string::npos)
            systemImage = true;
        else
            systemImage = false;
        baseAddress = IMG_LowAddress(img);
        endAddress = IMG_HighAddress(img);
        Image::imageList.push_back(*this);
        if (shortName.find("nine") != std::string::npos)
            mainImage = true;
        else
            mainImage = false;
    }
}

std::string tolower(std::string str)
{
    for (int i = 0; i < str.length(); i++)
    {
        if (str[i] >= 'A' && str[i] <= 'Z')
            str[i] += ('a' - 'A');
    }
    return str;
}

Image& Image::getImageByAddress(ADDRINT addr)
{
    for (int i = 0; i < Image::imageList.size(); i++)
    {
        if (addr >= imageList[i].getBaseAddress() && addr <= imageList[i].getEndAddress())
        {
            return imageList[i];
        }
    }
    return invalid;
}

Image& Image::getImageByName(std::string name)
{
    for (int i = 0; i < Image::imageList.size(); i++)
    {
        if (tolower(name) == tolower(imageList[i].getShortName()))
            return imageList[i];
    }
    return invalid;
}

void Image::writeMoudleInfo(std::string path)
{

    std::ofstream moduleInfo;
    moduleInfo.open(path.c_str());
    if (moduleInfo.is_open())
    {
        moduleInfo << "Image::writeMoudleInfo : " << hexstr((ADDRINT)&Image::writeMoudleInfo) << "\n";
        moduleInfo << std::setw(40) << "Module name" << std::setw(20) << "Start address" << std::setw(20) << "End address" << "\n";
        for (int i = 0; i < Image::imageList.size(); i++)
        {
            auto img = Image::imageList[i];
            moduleInfo << std::setw(40) << img.getShortName() << std::setw(20) << hexstr(img.getBaseAddress()) << std::setw(20)<< hexstr(img.getEndAddress());
            if (img.isSystemImage())            
                moduleInfo << "    SYSTEM";
            moduleInfo << std::endl;
        }
        moduleInfo.close();
    }
}

