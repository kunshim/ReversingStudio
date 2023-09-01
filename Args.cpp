#include "Args.h"

Args::Args(ADDRINT arg)
{
    if (PIN_CheckReadAccess((void*)arg))
    {
        ADDRINT nested;
        size_t copied = PIN_SafeCopy(&nested, (const void*)arg, sizeof(ADDRINT));
        if (copied == sizeof(ADDRINT))
        {
            type = TYPE::ARG_TYPE_PTRCHAIN;
            return;
        }
    }
    else
    {
        type = TYPE::ARG_TYPE_INT;
        return;
    }
}

Args::descript()
{

}

