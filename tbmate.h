/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2021 Wanding.Zhou@pennmedicine.upenn.edu
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
**/

#ifndef _TBMATE_H
#define _TBMATE_H

#include <stdint.h>

/*
 tbk file format:
 first 4 bytes: data_type_t
 */
#define PACKAGE_VERSION "0.2.0"

typedef enum {
  DT_NA, DT_INT2, DT_INT32, DT_FLOAT,
  DT_DOUBLE, DT_ONES} data_type_t;

#define TBK_HEADER_SIZE 4
#define MAX_DOUBLE16 ((1<<15)-2)

static inline float uint16_to_float(uint16_t i) {
  return ((float)i - MAX_DOUBLE16) / MAX_DOUBLE16;
}

static inline uint16_t float_to_uint16(float f) {
  return (uint16_t) ((f+1) * MAX_DOUBLE16);
}

#endif /* _TBMATE_H */
