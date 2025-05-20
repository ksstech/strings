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

// ########################################## Parse support ########################################

/**
 * @brief
 * @param[in]
 * @return
 */
int	xStringParseUnicode(char * pDst, char * pSrc, size_t Len);

/**
 * @brief
 * @param[in]
 * @return
 */
int	xStringParseEncoded(char * pDst, char * pSrc);

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
 * @brief	Parse value from buffer based on size/format/type specified
 * @param	pSrc - source pointer to take character(s) from
 * @param	cvI - format to expect input and to write parsed value in
 * @param	pX - pointer to destination buffer where value is to be stored
 * @return	Updated pointer or pcFAILURE
 */
char * cvParseValue(char * pSrc, cvi_e cvI, px_t pX);

/**
 * @brief		Parse range checked value from buffer based on size/format/type specified
 * @param[in]	pSrc - source pointer to take character(s) from
 * @param[in]	cvI - format to expect input and to write parsed value in
 * @param[out]	pX - pointer to destination buffer where value is to be stored
 * @param[in]	Lo - lowest allowed value
 * @param[in]	Hi - highest allowed value
 * @return		Updated pointer or pcFAILURE
 */
char * cvParseRangeX32(char * pSrc, px_t pX, cvi_e cvI, x32_t Lo, x32_t Hi);

/**
 * @brief		Same as cvParseRangeX32() except Lo and Hi are 64 bit values
 */
char * cvParseRangeX64(char * pSrc, px_t pX, cvi_e cvI, x64_t Lo, x64_t Hi);

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
 * @brief
 * @param[in]
 * @return
 */
void  x_string_general_test(void);

#ifdef __cplusplus
}
#endif
