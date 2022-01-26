#include "BKM10Rduino.h"

enum bank getBank(byte *b)
{
    if (strcmp((char *)b, "ILE") == 0)
    {
        return ILE;
    }

    if (strcmp((char *)b, "ISW") == 0)
    {
        return ISW;
    }

    if (strcmp((char *)b, "IMT") == 0)
    {
        return IMT;
    }

    if (strcmp((char *)b, "IEN") == 0)
    {
        return IEN;
    }

    if (strcmp((char *)b, "ICC") == 0)
    {
        return ICC;
    }

    if ((unsigned char)b[0] == 0x44)
    {
        return DATA;
    }

    return none;
}
