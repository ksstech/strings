/*
 * Copyright 2014-19 Andre M Maree / KSS Technologies (Pty) Ltd.
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
 * x_string_general.h
 */

#pragma once

#include	<stdint.h>
#include	<stddef.h>
#include	<stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ############################## Used for String <> DATETIME conversions ##########################

#define	DATETIME_YDAY_OK			BIT29MASK
#define	DATETIME_DOW_OK				BIT28MASK
#define	DATETIME_YEAR_OK			BIT27MASK
#define	DATETIME_MON_OK				BIT26MASK
#define	DATETIME_MDAY_OK			BIT25MASK
#define	DATETIME_HOUR_OK			BIT24MASK
#define	DATETIME_MIN_OK				BIT23MASK
#define	DATETIME_SEC_OK				BIT22MASK
#define	DATETIME_MSEC_OK			BIT21MASK
// Combination of flags for common test groups
#define	DATETIME_YMD_MASK			(DATETIME_YEAR_OK | DATETIME_MON_OK | DATETIME_MDAY_OK)
#define	DATETIME_HMS_MASK			(DATETIME_HOUR_OK | DATETIME_MIN_OK | DATETIME_SEC_OK)
#define	DATETIME_YMDHMS_MASK		(DATETIME_YMD_MASK | DATETIME_HMS_MASK)

// ########################################### MACROS ##############################################

// ##################################### string operations #########################################

typedef struct eTable_s {
	union {
		int			iVal1 ;
		uint32_t	uVal1 ;
	} ;
	union {
		int			iVal2 ;
		uint32_t	uVal2 ;
	} ;
	union {
		const char * pVoid ;
		const char * pMess ;
	} ;
} eTable_t ;

int	xstrverify(char * pStr, char cMin, char cMax, char cNum) ;
int	xstrlen(const char * s) ;
int	xstrnlen(const char * s, int len) ;
int	xstrncpy(char * s1, char * s2, int len ) ;

int	xmemrev(char * pMem, size_t Size) ;
void xstrrev(char * pStr) ;

int	xinstring(const char * pStr, char cChr) ;
int xstrncmp(const char * s1, const char * s2, size_t xLen, bool Exact) ;
int	xstrcmp(const char * s1, const char * s2, bool Exact) ;
int	xstrindex(char *, char * *) ;

int	xStringParseEncoded(char * pStr, char * pDst) ;
int	xStringSkipDelim(char * pSrc, const char * pDel, int MaxLen) ;
int	xStringFindDelim(char * pSrc, const char * pDlm, int xMax) ;
char * pcStringParseToken(char * pDst, char * pSrc, const char * pDel, int flag, int MaxLen) ;
struct tm ;
char * pcStringParseDateTime(char * buf, uint64_t * pTStamp, struct tm * ptm) ;
char * pcCodeToMessage(int eCode, const eTable_t * eTable) ;

int	xBitMapDecodeChanges(uint32_t Val1, uint32_t Val2, uint32_t Mask, const char * const pMesArray[], char * pBuf, size_t BufSize) ;
char * pcBitMapDecodeChanges(uint32_t Val1, uint32_t Val2, uint32_t Mask, const char * const pMesArray[]) ;

int	xBitMapDecode(uint32_t Value, uint32_t Mask, const char * const pMesArray[], char * pBuf, size_t BufSize) ;
char * pcBitMapDecode(uint32_t Value, uint32_t Mask, const char * const pMesArray[]) ;

void vBitMapDecode(uint32_t Value, uint32_t Mask, const char * const pMesArray[]) ;
void vBitMapReport(char * Name, uint32_t Value, uint32_t Width, const char ** pMesArray) ;
int	xStringValueMap(const char * pString, char * pBuf, uint32_t uValue, int iWidth) ;

void 	x_string_general_test(void) ;

#ifdef __cplusplus
}
#endif
