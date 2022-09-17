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
char * pcStringParseIpAddr(char * pStr, px_t px) ;

#ifdef __cplusplus
}
#endif
