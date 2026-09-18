#include "Converters.h"

namespace conversions
{

uint32_t UintToString(uint64_t var, char* outstr, uint8_t base, bool terminator)
{
    uint8_t buf[50]= {'\0'};
    uint8_t* str= buf +31;
    uint8_t c;
    do 
    {
        c = var % base;
        var /= base;
        *--str = c < 10 ? c + '0' : c + 'A' - 10;
    } while(var);

    for (int i = 0; i < 32; i++)
    {
        if( str[i] == 0 ) 
        {
            if (terminator)
            {
                outstr[i] = 0;
            }
            return i;
        }
        outstr[i] = str[i];
    }
    return 0;
}

uint32_t FloatToString(float var, char* str, uint8_t precision)
{
    int index = 0;
    if (var < 0.0) 
    {
        str[index++] = '-';
        var = -var;
    }

    float rounding = 0.5;
    for (uint8_t i = 0; i < precision; i++) 
        rounding /= 10.0;

    var += rounding;
    uint32_t intPart = static_cast<uint32_t>(var);
    float remainder = var - static_cast<float>(intPart);
    index += UintToString (intPart, &str[index], 10, false);
    str[index++] = '.';
    while (precision-- > 0)
    {
      remainder *= 10.0;
      uint16_t toPrint = (uint16_t)(remainder);
      index += UintToString (toPrint, &str[index], 10, false);
      remainder -= toPrint;
    }
    str[index] = 0;
    return index;
}


uint32_t IntToString(int64_t var, char* str, uint8_t base, bool terminator)
{
    int index = 0;
    if ((base == 10) && (var < 0) ) 
    {
        str[index++] = '-';
        var = -var;
    }

    uint32_t uintVar = static_cast<uint32_t> (var);
    return UintToString(uintVar, &str[index], base, terminator);
}

}   // namespace conversions
