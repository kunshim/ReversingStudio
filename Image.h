#pragma once
#include "pin.H"
#include <iostream>
#include <string>
#include <_unordered_map.h>
#include <fstream>
#include <sstream>
#include <time.h>

class Image
{
private:
    static std::vector<Image> imageList;
    ADDRINT baseAddress, endAddress;
    std::string shortName;
    std::string fullName;
    bool systemImage;
    bool mainImage;
    UINT32 id;
    std::string makeShortImgName(std::string name);
public:
    Image(UINT32 id);
    const std::string& getShortName() const
    {
        return shortName;
    }
    const std::string getFullName() const
    {
        return fullName;
    }
    bool isSystemImage() const
    {
        return systemImage;
    }
    bool isInvalid() const
    {
        return id == -1;
    }
    ADDRINT getBaseAddress() const
    {
        return baseAddress;
    }
    ADDRINT getEndAddress() const
    {
        return endAddress;
    }
    static Image& getImageByAddress(ADDRINT address);
    static Image& getImageByName(std::string name);
    static void writeMoudleInfo(std::string path);
    static Image invalid;
};