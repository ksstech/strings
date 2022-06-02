/*
 * Copyright (c) 2014-22 Andre M. Maree / KSS Technologies (Pty) Ltd.
 * x_string_general.h
 */

#pragma once

#include	"definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

// ########################################### MACROS ##############################################


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

// ####################################### enumeration #############################################

enum {
	bmdcCOLOUR	= (1 << 0),
	bmdcNEWLINE = (1 << 1),
};

// ##################################### string operations #########################################

int	xstrverify(char * pStr, char cMin, char cMax, char cNum);
int	xstrlen(const char * s);
int	xstrnlen(const char * s, int len);
int	xstrncpy(char * s1, char * s2, int len);

int	xmemrev(char * pMem, size_t Size);
void xstrrev(char * pStr);

int	xinstring(const char * pStr, char cChr);
int xstrncmp(const char * s1, const char * s2, size_t xLen, bool Exact);
int	xstrcmp(const char * s1, const char * s2, bool Exact);
int	xstrindex(char *, char * *);

int	xStringParseEncoded(char * pDst, char * pSrc);
int	xStringParseUnicode(char * pDst, char * pSrc, size_t sDst);
int	xStringSkipDelim(char * pSrc, const char * pDel, size_t sDst);
int	xStringFindDelim(char * pSrc, const char * pDlm, int xMax);
char * pcStringParseToken(char * pDst, char * pSrc, const char * pDel, int flag, size_t sDst);
char * pcStringParseDateTime(char * buf, u64_t * pTStamp, struct tm * ptm);

int	xBitMapDecodeChanges(u32_t Val1, u32_t Val2, u32_t Mask, const char * const pMesArray[], int Flag, char * pBuf, size_t BufSize);
char * pcBitMapDecodeChanges(u32_t Val1, u32_t Val2, u32_t Mask, const char * const pMesArray[], int Flag);

int	xBitMapDecode(u32_t Value, u32_t Mask, const char * const pMesArray[], char * pBuf, size_t BufSize);
char * pcBitMapDecode(u32_t Value, u32_t Mask, const char * const pMesArray[]);

void vBitMapDecode(u32_t Value, u32_t Mask, const char * const pMesArray[]);
void vBitMapReport(char * Name, u32_t Value, u32_t Width, const char ** pMesArray);
int	xStringValueMap(const char * pString, char * pBuf, u32_t uValue, int iWidth);

void  x_string_general_test(void);

#ifdef __cplusplus
}
#endif
