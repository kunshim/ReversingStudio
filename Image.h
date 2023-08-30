#pragma once
#include "pin.H"
#include <iostream>
#include <string>
#include <_unordered_map.h>
#include <fstream>
#include <sstream>

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
    std::string getShortName();
    std::string getFullName();
    bool isSystemImage();
    ADDRINT getBaseAddress();
    ADDRINT getEndAddress();
    static Image getImageByAddress(ADDRINT address);
    static Image getImageByName(std::string name);
    static void writeMoudleInfo(std::string path);
};

std::string toHexStr(std::string target);
