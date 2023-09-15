#include "Args.h"
//int APPLICATION_CP = SHIFT_JIS;
Args::Args(ADDRINT arg)
{
    int pid = PIN_GetPid();
    ADDRINT nested;
    this->arg = arg;
    EXCEPTION_INFO exptInfo;
    OS_MEMORY_AT_ADDR_INFORMATION info;
    OS_QueryMemory(pid, (void*)arg, &info);
    if (info.Protection != 0 && info.Protection <= 7)
    {
        OS_QueryMemory(pid, (void*)(*(ADDRINT*)arg), &info);
        if (info.Protection != 0 && info.Protection <= 7)
            type = TYPE::ARG_TYPE_PTRCHAIN;
        else
        {
            unsigned char *tmp = (unsigned char*)arg;
            for (int i = 0; i < 5; i++)
            {
                if (tmp[i] >= 0x80)
                {
                    type = TYPE::ARG_TYPE_WCHAR;
                    return; 
                }
            }
            type = TYPE::ARG_TYPE_CHAR;
        }
    }
    else
        type = TYPE::ARG_TYPE_INT;
}

void Args::descript()
{
    std::string tmp;
    wchar_t str[256] = {0,};
    switch(type)
    {
        case TYPE::ARG_TYPE_INT:
            std::cout << std::hex << arg << "\n";
            break;
        case TYPE::ARG_TYPE_CHAR:
            std::cout << "CHAR ";
            tmp = (const char*)arg;
            std::cout << tmp << "\n";
            break;
        case TYPE::ARG_TYPE_WCHAR:
            std::cout << "WCHAR ";
            std::cout << "Temp\n";
            break;
        case TYPE::ARG_TYPE_PTRCHAIN:
            std::cout << "PTRCHAIN ";
            std::cout << std::hex << arg << " -> "  << *(ADDRINT*)arg << "\n";
            break;
    }
}

