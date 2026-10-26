#pragma once
#include <stdint.h>
#include <cstddef>

namespace conversions
{

/**
 * @brief Conversion functions
 * @version Edited on 25.10.2026
 */

/**
 * @brief Float to string
 * @param [in] var Floating value
 * @param [out] str Saving string
 * @param [in] maxLen Limit of output string length
 * @param [in] precision Number of digits after '.'
 * @param [in] terminator Put the '\n' at the end
 * @return Result symbols count (without /0)
 */
uint32_t FloatToString(float val, char *outstr, size_t maxLen, uint8_t precision, bool terminator = true);

/**
 * @brief Unsigned long long to string
 * @param [in] var Value
 * @param [out] str Saving string
 * @param [in] maxLen Limit of output string length
 * @param [in] base Radix, base of the number system (2 for binary, 10 for dec, 16 for hex)
 * @param [in] terminator Put the '\n' at the end
 * @return Result symbols count (without /0)
 */
uint32_t UintToString(uint64_t var, char* outstr, size_t maxLen, uint8_t base = 10, bool terminator = true);

/**
 * @brief Signed long long to string
 * @param [in] var Value
 * @param [out] str Saving string
 * @param [in] maxLen Limit of output string length
 * @param [in] base Radix, base of the number system (2 for binary, 10 for dec, 16 for hex)
 * @param [in] terminator Put the '\n' at the end
 * @return Result symbols count (without /0)
 */
uint32_t IntToString(int64_t var, char* outstr, size_t maxLen, uint8_t base = 10, bool terminator = true);

}   // namespace conversions
