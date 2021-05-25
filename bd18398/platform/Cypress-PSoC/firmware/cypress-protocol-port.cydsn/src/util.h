// SPDX-License-Identifier: MIT
/*
 * The MIT License (MIT)
 * Copyright (c) 2021 ROHM Co.,Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file
 * @brief Miscellaneous utility tools.
 */
#ifndef UTIL_H
#define UTIL_H
#include <stddef.h>

/** Return the length of an array. */
#define UTIL_ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#define UTIL_SMALLER_OF(a, b) ((a) < (b) ? (a) : (b))

#define CEIL_DIV(A, B) (((A) + (B) - 1) / (B))

#define DIV_UINT_ROUND(dividend, divisor) ((dividend + divisor / 2) / divisor)

/* Delete an element of a volatile array.
 *
 * Effectively, a part of the array is just shifted. No actual memory is freed.
 */
void util_array_delete_v(volatile void *arr, size_t size, size_t index);

#endif				/* UTIL_H */
