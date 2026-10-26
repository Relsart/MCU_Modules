#include "Converters.h"

namespace conversions
{

uint32_t UintToString(uint64_t var, char* outstr, size_t maxLen, uint8_t base, bool terminator)
{
    if (!outstr || (maxLen == 1 && terminator) || base < 2 || base > 36)
        return 0;

    char buf[66]{0};        // Max value: uint64_t 0xFFFFFFFFFFFFFFFF in binary form (all '1')
    char* str = buf + 65;   // Starts from the end of local buffer
    *str = '\0';            // Mark end of local buffer with Null

    // Main generation cycle (converts whole value to local buffer without limitations):
    do
    {
        uint8_t c = var % base;
        var /= base;
        *--str = c < 10 ? c + '0' : c + 'A' - 10;

    } while(var);

    // Counting the length of the result string:
    uint32_t len = (buf + 65) - str;

    // How many symbols will be copied to destination address, according to maxLen limitations:
    uint32_t copy_len = len;
    if (maxLen > 0)
    {
        size_t availableSpace = terminator ? (maxLen - 1) : maxLen;
        if (copy_len > availableSpace)
            copy_len = availableSpace;
    }

    // Copying symbols to the destination buffer:
    for (uint32_t i = 0; i < copy_len; i++)
    {
        outstr[i] = str[i];
    }

    // Set Null-terminator if necessary:
    if (terminator)
        outstr[len] = '\0';
        
    return len;
}

uint32_t FloatToString(float val, char *outstr, size_t maxLen, uint8_t precision, bool terminator)
{
    if (!outstr || ((maxLen != 0) && (precision > 0) && (maxLen < 3)))
        return 0;  // Minimal string lenght: 3 chars (e.g. 1.1)

    size_t index = 0;
    bool unlimited = (maxLen == 0);
    
    // Define negative value sign:
    if (val < 0.0F)
    {
        if (unlimited || maxLen > 1)
            outstr[index++] = '-';
        val = -val;
    }

    // Rounding value:
    float rounding = 0.5f;
    for (uint8_t p = 0; p < precision; p++)
        rounding /= 10.0f;
    val += rounding;

    // Division into integer and fractional parts:
    uint32_t intPart = static_cast<uint32_t>(val);
    float flPart = val - static_cast<float>(intPart);

    // Integer part output:
    size_t limit = unlimited ? 0 : (maxLen - index);
    // ACHTUNG: UintToString returns FULL necessary (as like it's unlimited) number of chars:
    uint32_t fullSize = UintToString(intPart, &outstr[index], limit, 10, false);
    index += (!unlimited && fullSize > limit) ? limit : fullSize;

    // Floating part output:
    if (precision > 0 && (unlimited || index < maxLen - 1))
    {
        outstr[index++] = '.';

        // Converting floating part into integer
        for (uint8_t p = 0; p < precision; p++)
            flPart *= 10.0f;

        uint32_t fpartInt = (uint32_t)flPart;

        // Counting number of digits in floating part:
        uint32_t temp = fpartInt;
        uint8_t digits_count = 0;
        do
        {
            digits_count++;
            temp /= 10;
        } while(temp);

        // Pushing leading zeroes straight to the buffer (e.g. "000" from 0.00012):
        while (digits_count < precision && (unlimited || index + (terminator ? 1 : 0) < maxLen))
        {
            outstr[index++] = '0';
            digits_count++;
        }

        // Floating part (its significant part) output:
        limit = unlimited ? 0 : (maxLen - index);
        fullSize = UintToString(fpartInt, &outstr[index], limit, 10, terminator);
        
        // Moving index by the number of characters actually written:
        if (!unlimited && fullSize > limit)
            index += terminator ? (limit > 0 ? limit - 1 : 0) : limit;
        else
            index += fullSize;
    }
    else if (terminator)  // precision == 0, no fraq part needed:
    {
        // If terminator is needed to be placed, but there are no free place- remove the last digit (null MUST be placed anyway!)
        if (unlimited || index < maxLen)
            outstr[index] = '\0';
        else if (maxLen > 0)
            outstr[maxLen - 1] = '\0';
    }

    return index;
}

uint32_t IntToString(int64_t var, char* outstr, size_t maxLen, uint8_t base, bool terminator)
{
    if (!outstr || (maxLen > 0 && terminator && maxLen < 2) || base < 2 || base > 36)
        return 0;

    size_t index = 0;
    bool unlimited = (maxLen == 0);
    uint64_t absValue;

    // Negative case handling:
    if (var < 0)
    {
        if (unlimited || index < (maxLen - (terminator ? 1 : 0)))
            outstr[index++] = '-';
        
        // Correctly processing denormalized value:
        absValue = (var == INT64_MIN) ? (uint64_t)INT64_MAX + 1ULL : (uint64_t)(-var);
    }
    else
    {
        absValue = (uint64_t)var;
    }

    // Unsigned integer conversion:
    size_t limit = unlimited ? 0 : (maxLen - index);
    uint32_t printed = UintToString(absValue, &outstr[index], limit, base, terminator);

    // Get the final chars count:
    if (!unlimited && printed > limit)
        index += terminator ? (unlimited ? 0 : limit - 1) : limit;
    else
        index += printed;

    return index;
}

}   // namespace conversions
