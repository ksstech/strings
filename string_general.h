// string_general.h

#pragma once

#include "struct_union.h"
#include "definitions.h"
#include "timeX.h"

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ########################################### MACROS ##############################################

#define	controlSIZE_FLAGS_BUF		(24 * 10)

#define stringXMEMREV_XOR           0

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

// ##################################### string operations #########################################

int	xstrverify(char * pStr, char cMin, char cMax, char cNum);
size_t xstrnlen(const char * s, size_t len);
int	xstrncpy(char * s1, char * s2, int len);

int	xmemrev(char * pMem, size_t Size);
void xstrrev(char * pStr);

int	strchr_i(const char * pStr, char cChr);
int xstrncmp(const char * s1, const char * s2, size_t xLen, bool Exact);
int	xstrcmp(const char * s1, const char * s2, bool Exact);
int	xstrindex(char *, char * *);
int xstrishex(char *);

int	xStringParseEncoded(char * pDst, char * pSrc);
int	xStringParseUnicode(char * pDst, char * pSrc, size_t sDst);
int	xStringSkipDelim(char * pSrc, const char * pDel, size_t sDst);
int xStringCountSpaces(char * pSrc);
int xStringCountCRLF(char * pSrc);
int	xStringFindDelim(char * pSrc, const char * pDlm, size_t xMax);

char * pcStringParseToken(char * pDst, char * pSrc, const char * pDel, int flag, size_t sDst);
char * pcStringParseDateTime(char * buf, u64_t * pTStamp, tm_t * psTM);

struct report_t;
int	xBitMapDecodeChanges(struct report_t * psR, u32_t V1, u32_t V2, u32_t Mask, const char * const pMesArray[]);
int	xStringValueMap(const char * pString, char * pBuf, u32_t uValue, int iWidth);

void  x_string_general_test(void);

#ifdef __cplusplus
}
#endif
