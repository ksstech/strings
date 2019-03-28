/*
 * Copyright 2014-18 Andre M Maree / KSS Technologies (Pty) Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * x_string_to_values.h
 */

#pragma once

#include	"x_complex_vars.h"

#ifdef __cplusplus
extern "C" {
#endif

// ################################# x_string_to_values ############################################

uint64_t char2u64(uint8_t *, uint64_t *, uint32_t) ;
uint64_t xStringParseX64(char *pSrc, uint8_t * pDst, uint32_t xLen) ;
int32_t	xHexCharToValue(uint8_t, int32_t) ;

char *	pcStringParseU64(char * pSrc, uint64_t * pVal, int32_t * pSign, const char * pDel) ;
char *	pcStringParseF64(char *str, double * pDst, int32_t * pSign, const char * pDel) ;
char *	pcStringParseX64(char * pSrc, x64_t * px64Val, varform_t VarForm, const char * pDel) ;
char *	pcStringParseValue(char * pSrc, p32_t p32Pntr, varform_t VarForm, varsize_t VarSize, const char * pDel) ;
char *	pcStringParseValueRange(char * pSrc, p32_t p32Pntr, varform_t VarForm, varsize_t VarSize, const char * pDel, x32_t x32Lo, x32_t x32Hi) ;
char *	pcStringParseValues(char * pSrc, p32_t p32Pntr, varform_t VarForm, varsize_t VarSize, const char * pDel, int32_t Count) ;
char *	pcStringParseNumber(int32_t * i32Ptr, char * pSrc) ;
char *	pcStringParseNumberRange(int32_t * i32Ptr, char * pSrc, int32_t Min, int32_t Max) ;
char *	pcStringParseIpAddr(char * pStr, uint32_t * pVal) ;

void	x_string_values_test(void) ;

#ifdef __cplusplus
}
#endif
