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

/**
 * @brief	verify to a maximum number of characters that each character is within a range
 * @return	erSUCCESS if cNum or fewer chars tested OK and a NUL is reached
 */
int	xstrverify(char * pStr, char cMin, char cMax, char cNum);

/**
 * @brief	calculate the length of the string up to max len specified
 * @param	s		pointer to the string
 * @param	len		maximum length to check for/return
 * @return	length of the string excl the terminating '\0'
 */
size_t xstrnlen(const char * s, size_t len);

/**
 * @brief	Copies from s2 to s1 until either 'n' chars copied or '\000' reached in s2
 * 			String will ONLY be terminated if less than maximum characters copied.
 * @param	s1 - pointer to destination uint8_t array (string)
 *			s2 - pointer to source uint8_t array (string)
 *			n - maximum number of chars to copy
 * @return	Actual number of chars copied (x <= n) excluding possible NULL
 */
int	xstrncpy(char * s1, char * s2, int len);

int	xmemrev(char * pMem, size_t Size);

/**
 * @param  str is a pointer to the string to be reversed
 * @return none
 * @brief  perform 'in-place' start <-> end character reversal
 */
void xstrrev(char * pStr);

/**
 * @brief	determine position of character in string (if at all)
 * @param	pStr pointer to string to scan
 * @param	cChr character match to find
 * @return	0 -> n the index into the string where the char is found
 * 			FAILURE if no match found, or cChr is NULL
 */
 int strchr_i(const char * pStr, int iChr);

 /**
 * @brief	based on flag case in/sensitive
 * @param	s1/2 - pointers to strings to be compared
 * @param	xLen - maximum length to compare (non null terminated string)
 * @param	flag - true for exact match, else upper/lower case difference ignored
 * @return			true or false based on comparison
 */
int xstrncmp(const char * s1, const char * s2, size_t xLen, bool Exact);

/**
 * @brief	compare two strings based on flag case in/sensitive
 * @param	s1, s2 - pointers to strings to be compared
 * 			flag - true for exact match, else upper/lower case difference ignored
 * @return	true or false based on comparison
 */
int	xstrcmp(const char * s1, const char * s2, bool Exact);

/**
 * @brief	determine array index of specified string in specified string array
 * 			expects the array to be null terminated
 * @param	key	- pointer to key string to find
 * 			table - pointer to array of pointers to search in.
 * 			flag - true for exact match, else upper/lower case difference ignored
 * @return	if match found, index into array else FAILURE
 */
int	xstrindex(char *, char * *);

/**
 * @brief
 * @param[in]
 */
int xstrishex(char *);

/**
 * @brief	parse an encoded string (optionally in-place) to a non-encoded string
 * @return	erFAILURE if illegal/unexpected characters encountered
 * 			erSUCCESS if a zero length string encountered
 * 			1 or greater = length of the parsed string
 */
int	xStringParseEncoded(char * pDst, char * pSrc);
int	xStringParseUnicode(char * pDst, char * pSrc, size_t sDst);

/**
 * @brief	skips specified delimiters (if any)
 * @brief	does NOT automatically work on assumption that string is NULL terminated, hence requires MaxLen
 * @brief	ONLY if MaxLen specified as NULL, assume string is terminated and calculate length.
 * @param[in]	pSrc - pointer to source buffer
 * @param[in]	pDel - pointer to string of valid delimiters
 * @param[in]	MaxLen - maximum number of characters in buffer
 * @return		number of delimiters (to be) skipped
 */
int	xStringSkipDelim(char * pSrc, const char * pDel, size_t sDst);
int xStringCountSpaces(char * pSrc);
int xStringCountCRLF(char * pSrc);
int	xStringFindDelim(char * pSrc, const char * pDlm, size_t xMax);

/**
 * @brief	Copies token from source buffer to destination buffer
 * @param	pDst - pointer to destination buffer
 * @param	pSrc - pointer to source buffer
 * @param	pDel - pointer to possible (leading) delimiters (to be skipped)
 * @param	flag - (< 0) force lower case, (= 0) no conversion (> 0) force upper case
 * @param	MaxLen - maximum number of characters in buffer
 * @return	pointer to next character to be processed...
 */
char * pcStringParseToken(char * pDst, char * pSrc, const char * pDel, int flag, size_t sDst);

/**
 * pcStringParseDateTime()
 * @brief		parse a string with format	2015-04-01T12:34:56.789Z
 * 											2015/04/01T23:34:45.678Z
 * 											2015-04-01T12h34m56s789Z
 * 											2015/04/01T23h12m54s123Z
 * 											CCYY?MM?DD?hh?mm?ss?uuuuuu?
 * 				to extract the date/time components x.789Z mSec components are parsed and stored if found.
 * @param[in]	buf - pointer to the string to be parsed
 * @param[out]	pTStamp - pointer to structure for epoch seconds [+millisec]
 * @param[out]	ptm - pointer to structure of type tm
 * @return		updated buf pointing to next character to be processed
 * 				valid values (if found) in time structure
 * 				0 if no valid milli/second values found
 * @note		tm_isdst not yet calculated/set!!
 * 				Must also add support for following 3 formats as used
 * 				in HTTP1.1 onwards for "If-Modified-Since" option
 * 					Fri, 31 Dec 1999 23:59:59 GMT (standard going forward)
 *					Friday, 31-Dec-99 23:59:59 GMT
 *					Fri Dec 31 23:59:59 1999
 */
char * pcStringParseDateTime(char * buf, u64_t * pTStamp, tm_t * psTM);

/**
 * @brief	build an output string using bit-mapped mask to select characters from a source string
 * @brief	with source string "ABCDEFGHIJKLMNOPQRST" and value 0x000AAAAA will build "A-C-E-G-I-K-M-O-Q-S-"
 * @param	pString
 * @param	pBuf
 * @param	uValue
 * @param	iWidth
 * @return
 */
int	xStringValueMap(const char * pString, char * pBuf, u32_t uValue, int iWidth);

void  x_string_general_test(void);

#ifdef __cplusplus
}
#endif
