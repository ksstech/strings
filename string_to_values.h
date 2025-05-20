// string_to_values.h

#pragma once

#include "struct_union.h"

#ifdef __cplusplus
extern "C" {
#endif

// ################################### Public functions ############################################

u64_t char2u64(char *, u64_t *, int) ;
u64_t xStringParseX64(char *, char *, int) ;
int	xHexCharToValue(char, int);
int xSumHexCharToValue(char cChr, u8_t * pU8);
int xParseHexString(char * pSrc, u8_t * pU8, size_t sU8);

char * pcStringParseIpAddr(char * pStr, px_t px);

#ifdef __cplusplus
}
#endif
