#pragma once
#include <stdint.h>

namespace conversions
{

/**
 * @brief Some conversion functions
 * @details Standard functions (such as sprintf) may failed working with uint64_t values, so using this:
 */

/**
 * @brief Float to string
 * @param [in] var Floating value
 * @param [out] str Saving string
 * @param [in] precision Precision
 * @return Symbols count (without /0)
 */
uint32_t FloatToString(float var, char* str, uint8_t precision = 3);

/**
 * @brief Unsigned long long to string
 * @param [in] var Value
 * @param [out] str Saving string
 * @param [in] base May be 2, 8, 10 or 16
 * @param [in] terminator Is needed /0 at the end?
 * @return Symbols count (without /0)
 */
uint32_t UintToString(uint64_t var, char* str, uint8_t base = 10, bool terminator = true);

/**
 * @brief Signed long long to string
 * @param [in] var Value
 * @param [out] str Saving string
 * @param [in] base May be 2, 8, 10 or 16
 * @param [in] terminator Is needed /0 at the end?
 * @return Symbols count (without /0)
 */
uint32_t IntToString(int64_t var, char* str, uint8_t base = 10, bool terminator = true);

}   // namespace conversions
