/*
 * Copyright (c) 2014-22 Andre M. Maree / KSS Technologies (Pty) Ltd.
 * x_string_to_values.h
 */

#pragma once

#include	"complex_vars.h"		// struct_union x_time definitions stdint time

#ifdef __cplusplus
extern "C" {
#endif

// ################################### Public functions ############################################

u64_t char2u64(char *, u64_t *, int) ;
u64_t xStringParseX64(char *, char *, int) ;
int	xHexCharToValue(char, int) ;
char * pcStringParseU64(char * pSrc, u64_t * pVal, int * pSign, const char * pDel) ;
char * pcStringParseF64(char *pSrc, double * pDst, int * pSign, const char * pDel) ;
char * pcStringParseX64(char * pSrc, x64_t * px64Val, vf_e VarForm, const char * pDel) ;
char * pcStringParseValue(char * pSrc, px_t px, vf_e VarForm, vs_e VarSize, const char * pDel) ;
char * pcStringParseParam(char * pSrc, px_t pX, cvi_e cvI) ;
char * pcStringParseValueRange(char * pSrc, px_t px, vf_e VarForm, vs_e VarSize, const char * pDel, x32_t x32Lo, x32_t x32Hi) ;
char * pcStringParseValues(char * pSrc, px_t px, vf_e VarForm, vs_e VarSize, const char * pDel, int Count) ;
char * pcStringParseNumber(char * pSrc, px_t px) ;
char * pcStringParseNumberRange(char * pSrc, px_t px, int Min, int Max) ;
char * pcStringParseIpAddr(char * pStr, px_t px) ;
void x_string_values_test(void) ;

#ifdef __cplusplus
}
#endif
