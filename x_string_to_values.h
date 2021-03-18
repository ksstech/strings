/*
 * Copyright 2014-21 Andre M. Maree / KSS Technologies (Pty) Ltd.
 */

/*
 * x_string_to_values.h
 */

#pragma once

#include	"x_complex_vars.h"

#ifdef __cplusplus
extern "C" {
#endif

// ################################### Public functions ############################################

uint64_t char2u64(uint8_t *, uint64_t *, uint32_t) ;
uint64_t xStringParseX64(char *pSrc, uint8_t * pDst, uint32_t xLen) ;
int32_t	xHexCharToValue(uint8_t, int32_t) ;

char *	pcStringParseU64(char * pSrc, uint64_t * pVal, int32_t * pSign, const char * pDel) ;
char *	pcStringParseF64(char *pSrc, double * pDst, int32_t * pSign, const char * pDel) ;

char *	pcStringParseX64(char * pSrc, x64_t * px64Val, varform_t VarForm, const char * pDel) ;
char *	pcStringParseValue(char * pSrc, px_t px, varform_t VarForm, varsize_t VarSize, const char * pDel) ;
char *	pcStringParseValueRange(char * pSrc, px_t px, varform_t VarForm, varsize_t VarSize, const char * pDel, x32_t x32Lo, x32_t x32Hi) ;
char *	pcStringParseValues(char * pSrc, px_t px, varform_t VarForm, varsize_t VarSize, const char * pDel, int32_t Count) ;
char *	pcStringParseNumber(char * pSrc, px_t px) ;
char *	pcStringParseNumberRange(char * pSrc, px_t px, int32_t Min, int32_t Max) ;
char *	pcStringParseIpAddr(char * pStr, px_t px) ;

void	x_string_values_test(void) ;

#ifdef __cplusplus
}
#endif
