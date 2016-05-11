/**
 * @file
 *
 * Define atomic read-modify-write memory options for Microsoft compiler
 */

/******************************************************************************
 *
 *
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
#ifndef _TOOLCHAIN_QCC_ATOMIC_H
#define _TOOLCHAIN_QCC_ATOMIC_H

#include <windows.h>

namespace qcc {

/**
 * Increment an int32_t and return its new value atomically.
 *
 * @param mem  Pointer to int32_t to be incremented.
 * @return  New value (after increment) of *mem
 */
inline int32_t IncrementAndFetch(volatile int32_t* mem) {
    return InterlockedIncrement(reinterpret_cast<volatile long*>(mem));
}

/**
 * Decrement an int32_t and return its new value atomically.
 *
 * @param mem  Pointer to int32_t to be decremented.
 * @return  New value (after decrement) of *mem
 */
inline int32_t DecrementAndFetch(volatile int32_t* mem) {
    return InterlockedDecrement(reinterpret_cast<volatile long*>(mem));
}

/**
 * Performs an atomic compare-and-exchange operation on the specified values.
 * It compares two specified 32-bit values and exchanges with another 32-bit
 * value based on the outcome of the comparison.
 *
 * @param mem  Pointer to int32_t to be compared and modified.
 * @param expectedValue  Expected value of *mem.
 * @param newValue  New value of *mem after calling this function, if returning true.
 * @return  true if the initial value of *mem was expectedValue, false otherwise
 */
inline bool CompareAndExchange(volatile int32_t* mem, int32_t expectedValue, int32_t newValue) {
    return (InterlockedCompareExchange(reinterpret_cast<volatile long*>(mem), newValue, expectedValue) == expectedValue);
}

/**
 * Performs an atomic compare-and-exchange operation on the specified pointer values.
 * It compares two specified pointer values and exchanges with another pointer
 * value based on the outcome of the comparison.
 *
 * @param mem  Pointer to the pointer value to be compared and modified.
 * @param expectedValue  Expected value of *mem.
 * @param newValue  New value of *mem after calling this function, if returning true.
 * @return  true if the initial value of *mem was expectedValue, false otherwise
 */
inline bool CompareAndExchangePointer(void* volatile* mem, void* expectedValue, void* newValue) {
    return (InterlockedCompareExchangePointer(mem, newValue, expectedValue) == expectedValue);
}

}

#endif
