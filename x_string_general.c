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
 * x_string_general.c
 */

#include	"x_string_general.h"
#include	"x_string_to_values.h"
#include	"x_errors_events.h"
#include	"x_time.h"
#include	"syslog.h"
#include	"printfx.h"

#include	"hal_config.h"
#include	"hal_debug.h"

#include	<string.h>
#include	<ctype.h>

#define	debugFLAG						0xC000

#define	debugXSTRCMP					(debugFLAG & 0x0001)
#define	debugPARSE_U64					(debugFLAG & 0x0002)
#define	debugPARSE_F64					(debugFLAG & 0x0004)
#define	debugPARSE_X64					(debugFLAG & 0x0008)

#define	debugPARSE_VALUE				(debugFLAG & 0x0010)
#define	debugPARSE_TOKEN				(debugFLAG & 0x0020)
#define	debugPARSE_DTIME				(debugFLAG & 0x0040)
#define	debugPARSE_TRACK				(debugFLAG & 0x0080)

#define	debugPARSE_ENCODED				(debugFLAG & 0x0100)
#define	debugDELIM						(debugFLAG & 0x0200)

#define	debugPARAM						(debugFLAG & 0x4000)
#define	debugRESULT						(debugFLAG & 0x8000)

/**
 * xstrverify() - verify to a maximum number of characters that each character is within a range
 * @return	erSUCCESS if cNum or fewer chars tested OK and a CHR_NUL is reached
 */
int32_t	xstrverify(char * pStr, char cMin, char cMax, char cNum) {
	if (*pStr == CHR_NUL) {			// if the 1st character is a NUL, ie no ASCII string there
		return erFAILURE ;			// return error
	}
	while (cNum--) {
		if (OUTSIDE(cMin, *pStr, cMax, char)) {
			return erFAILURE ;
		}
	}
	return erSUCCESS ;
}

/**
 * xstrnlen() - calculate the length of the string up to max len specified
 * @param[in]	s		pointer to the string
 * @param[in]	len		maximum length to check for/return
 * @return		length of the string excl the terminating '\0'
 */
int32_t	xstrnlen(char * s, int32_t len) {
	int32_t l = 0;
	PRINT("s=%s  len=%d", s, len) ;
	while ((*s != CHR_NUL) && (l < len)) {
		s++ ;
		l++ ;
	}
	PRINT("  l=%d\n", l) ;
	IF_myASSERT(debugRESULT, l < len) ;
	return l ;
}

/**
 * xstrlen
 * @brief	calculate the length of the string
 * @param	char *s		pointer to the string
 * @return	int32_t		length of the string up to but excl the terminating '\0'
 */
int32_t	xstrlen(char * s) { return xstrnlen(s, 32767) ; }

/**
 * xstrncpy
 * @brief	Copies from s2 to s1 until either 'n' chars copied or '\000' reached in s2
 * 			String will ONLY be terminated if less than maximum characters copied.
 * @param	s1 - pointer to destination uint8_t array (string)
 *			s2 - pointer to source uint8_t array (string)
 *			n - maximum number of chars to copy
 * @return	Actual number of chars copied (x <= n) excluding possible NULL
 */
int32_t	xstrncpy(char * pDst, char * pSrc, int32_t xLen ) {
	IF_myASSERT(debugPARAM, INRANGE_SRAM(pDst) && INRANGE_MEM(pSrc) && xLen) ;
	int32_t Cnt = 0 ;
	while ((*pSrc != CHR_NUL) && (Cnt < xLen)) {
		*pDst++ = *pSrc++ ;						// copy across and adjust both pointers
		Cnt++ ;								// adjust length copied
	}
	if (Cnt < xLen) {						// ONLY if less than maximum copied...
		*pDst = CHR_NUL ;					// null terminate the string
	}
	return Cnt ;
}

int32_t	xmemrev(char * pMem, size_t Size) {
	IF_myASSERT(debugPARAM, INRANGE_SRAM(pMem) && Size > 1) ;
	if (pMem == NULL || *pMem == CHR_NUL || Size < 2) {
		return erFAILURE;
	}
	char * pRev = pMem + Size - 1 ;
#if		(stringXMEMREV_XOR == 1)
	for (char * pFwd = pMem; pRev > pFwd; ++pFwd, --pRev) {
		*pFwd ^= *pRev ;
		*pRev ^= *pFwd ;
		*pFwd ^= *pRev ;
	}
#else
	while (pMem < pRev) {
		char cTemp = *pMem ;
		*pMem++	= *pRev ;
		*pRev--	= cTemp ;
	}
#endif
	return erSUCCESS ;
}

/**
 * xstrrev
 * @param  str is a pointer to the string to be reversed
 * @return none
 * @brief  perform 'in-place' start <-> end character reversal
 */
void	xstrrev(char * pStr) {
#if 0
	IF_myASSERT(debugPARAM, INRANGE_SRAM(pStr)) ;
	if ((pStr == NULL) || (*pStr == CHR_NUL)) {
		return ;
	}
	char * pRev = pStr + xstrlen(pStr) - 1 ;
	for (char * pFwd = pStr; pRev > pFwd; ++pFwd, --pRev) {
		*pFwd ^= *pRev ;
		*pRev ^= *pFwd ;
		*pFwd ^= *pRev ;
	}
#else
	xmemrev(pStr, xstrlen(pStr)) ;
#endif
}

/**
 * xinstring()
 * @brief		determine position of character in string (if at all)
 * @param		pStr - pointer to string to scan
 * 				cChr - the character match to find
 * @return		0 -> n the index into the string where the char is found
 * 				FAILURE if no match found, or cChr is NULL
 */
int32_t	xinstring(const char * pStr, char cChr) {
	IF_myASSERT(debugPARAM, INRANGE_MEM(pStr)) ;
	if (cChr == CHR_NUL) {
		return erFAILURE ;
	}
	int32_t	pos = 0 ;
	while (*pStr) {
		if (*pStr++ == cChr) {
			return pos ;
		}
		pos++ ;
	}
	return erFAILURE ;
}

/**
 * xstrncmp() compare two strings
 * @brief	 based on flag case in/sensitive
 * @param s1, s2	pointers to strings to be compared
 * @param xLen		maximum length to compare (non null terminated string)
 * @param flag		true for exact match, else upper/lower case difference ignored
 * @return			true or false based on comparison
 */
int32_t	xstrncmp(const char * s1, const char * s2, size_t xLen, bool Exact) {
	IF_myASSERT(debugPARAM, INRANGE_MEM(s1) && INRANGE_MEM(s2) && xLen < 1024) ;
	IF_SL_DBG(debugXSTRCMP, "xLen=%d '%.*s' vs '%.*s' ", xLen, xLen, s1, xLen, s2) ;
	while (*s1 && *s2 && xLen) {
		if (Exact == true) {
			if (*s1 != *s2) {
				break ;
			}
		} else {
			if (toupper((int)*s1) != toupper((int)*s2)) {
				break ;
			}
		}
		++s1 ;
		++s2 ;
		--xLen ;
	}
	return ((*s1 == CHR_NUL) && (*s2 == CHR_NUL)) ? true : (xLen == 0) ? true : false ;
}

/**
 * xstrcmp() compare two strings
 * @brief	 based on flag case in/sensitive
 * @param	s1, s2 - pointers to strings to be compared
 * 			flag - true for exact match, else upper/lower case difference ignored
 * @return	true or false based on comparison
 */
int32_t	xstrcmp(const char * s1, const char * s2, bool Exact) {
	IF_myASSERT(debugPARAM, INRANGE_MEM(s1) && INRANGE_MEM(s2)) ;
	IF_SL_DBG(debugXSTRCMP, " S1=%s:S2=%s ", s1, s2) ;
	while (*s1 && *s2) {
		if (Exact) {
			if (*s1 != *s2) {
				break ;
			}
		} else {
			if (toupper((int)*s1) != toupper((int)*s2)) {
				break ;
			}
		}
		++s1 ;
		++s2 ;
	}
	return ((*s1 == CHR_NUL) && (*s2 == CHR_NUL)) ? true : false ;
}

/**
 * xstrindex
 * @brief	determine array index of specified string in specified string array
 * 			expects the array to be null terminated
 * @param	key	- pointer to key string to find
 * 			table - pointer to array of pointers to search in.
 * 			flag - true for exact match, else upper/lower case difference ignored
 * @return	if match found, index into array else FAILURE
 */
int32_t	xstrindex(char * key, char * array[]) {
	int32_t	i = 0 ;
	while (array[i]) {
		if (strcasecmp(key, array[i]) == 0) {			// strings match?
			return i ;									// yes, return the index
		}
		++i ;
	}
	return erFAILURE ;
}

// ############################### string parsing routines #########################################

/**
 * xStringParseEncoded()
 * @brief	parse an encoded string (optionally in-place) to a non-encoded string
 * @return	erFAILURE if illegal/unexpected characters encountered
 * 			erSUCCESS if a zero length string encountered
 * 			1 or greater = length of the parsed string
 */
int32_t	xStringParseEncoded(char * pStr, char * pDst) {
	IF_myASSERT(debugPARAM, INRANGE_SRAM(pStr)) ;
	int32_t iRV = 0 ;
	if (pDst == NULL) {
		pDst	= pStr ;
	} else {
		IF_myASSERT(debugPARAM, INRANGE_SRAM(pDst)) ;
	}
	IF_PRINT(debugPARSE_ENCODED, "%s  ", pStr) ;
	while(*pStr != 0) {
		if (*pStr == CHR_PERCENT) {						// escape char?
			int32_t Val1 = xHexCharToValue(*++pStr, BASE16) ;	// yes, parse 1st value
			if (Val1 == erFAILURE) {
				return erFAILURE ;
			}
			int32_t Val2 = xHexCharToValue(*++pStr, BASE16) ;	// parse 2nd value
			if (Val2 == erFAILURE) {
				return erFAILURE ;
			}
			IF_PRINT(debugPARSE_ENCODED, "[%d+%d=%d]  ", Val1, Val2, (Val1 << 4) + Val2) ;
			*pDst++ = (Val1 << 4) + Val2 ;				// calc & store final value
			++pStr ;									// step to next char
		} else {
			*pDst++ = *pStr++ ;							// copy as is to (new) position
		}
		++iRV ;										// & adjust count...
	}
	*pDst = CHR_NUL ;									// terminate
	IF_PRINT(debugPARSE_ENCODED, "%s\n", pStr-iRV) ;
	return iRV ;
}

#define	stringGENERAL_MAX_LEN		2048

/**
 * xStringSkipDelim() - skips specified delimiters (if any)
 * @brief		does NOT automatically work on assumption that string is NULL terminated, hence requires MaxLen
 * @brief		ONLY if MaxLen specified as NULL, assume string is terminated and calculate length.
 * @param[in]	pSrc - pointer to source buffer
 * @param[in]	pDel - pointer to string of valid delimiters
 * @param[in]	MaxLen - maximum number of characters in buffer
 * @return		number of delimiters skipped
 */
int32_t	xStringSkipDelim(char * pSrc, const char * pDel, int32_t MaxLen) {
	IF_myASSERT(debugPARAM, INRANGE_MEM(pSrc) && INRANGE_MEM(pDel)) ;
	// If no length supplied
	if (MaxLen == 0) {
		MaxLen = xstrnlen(pSrc, stringGENERAL_MAX_LEN) ;// assume NULL terminated and calculate length
		IF_myASSERT(debugRESULT, MaxLen < stringGENERAL_MAX_LEN) ;		// just a check to verify not understated
	}

	IF_PRINT(debugDELIM, " '%.4s'", pSrc) ;
	// continue skipping over valid terminator characters
	int32_t	CurLen = 0 ;
	while ((xinstring(pDel, *pSrc) != erFAILURE) && (CurLen < MaxLen)) {
		++pSrc ;
		++CurLen ;
	}

	IF_PRINT(debugDELIM, "->'%.4s'", pSrc) ;
	return CurLen ;								// number of delimiters skipped over
}

int32_t	xStringFindDelim(char * pSrc, const char * pDlm, int32_t xMax) {
	IF_myASSERT(debugPARAM, INRANGE_MEM(pSrc) && INRANGE_FLASH(pDlm)) ;
	int32_t xPos = 0 ;
	if (xMax == 0) {
		xMax = strlen(pSrc) ;
	}
	while (*pSrc && xMax) {
		int32_t	xSrc = isupper((int) *pSrc) ? tolower((int) *pSrc) : (int) *pSrc ;
		const char * pTmp = pDlm ;
		while (*pTmp) {
			int32_t	xDlm = isupper((int) *pTmp) ? tolower((int) *pTmp) : (int) *pTmp ;
			if (xSrc == xDlm) {
				return xPos ;
			}
			++pTmp ;
		}
		++xPos ;
		++pSrc ;
		--xMax ;
	}
	return erFAILURE ;
}

/**
 * pcStringParseToken() - copies next single token from source buffer to destination buffer
 * @brief		does NOT automatically work on assuption that string is NULL terminated, hence requires MaxLen
 * @brief		if MaxLen specified as NULL, assume string is terminated and calculate length.
 * @brief		skips specified delimiters (if any)
 * @param[in]	pDst - pointer to destination buffer
 * @param[in]	pSrc - pointer to source buffer
 * @param[in]	pDel - pointer to string of valid delimiters
 * @param[in]	flag - (< 0) force lower case, (= 0) no conversion (> 0) force upper case
 * @param[in]	MaxLen - maximum number of characters in buffer
 * @return		pointer to next character to be processed...
 */
char *	pcStringParseToken(char * pDst, char * pSrc, const char * pDel, int32_t flag, int32_t MaxLen) {
	IF_myASSERT(debugPARAM, INRANGE_SRAM(pDst) && INRANGE_MEM(pSrc) && INRANGE_MEM(pDel) && (*pSrc != CHR_NUL) && (*pDel != CHR_NUL)) ;
	// If no length supplied
	if (MaxLen == 0) {
		MaxLen = strlen((const char *) pSrc) ;			// assume NULL terminated and calculate length
	}
	int32_t CurLen = xStringSkipDelim(pSrc, pDel, MaxLen) ;
	MaxLen	-= CurLen ;
	pSrc	+= CurLen ;

	while (*pSrc && MaxLen--) {							// while not separator/terminator char or end of string
		if (xinstring(pDel, *pSrc) != erFAILURE)	{	// check if current char a delim
			break ;										// yes, all done...
		}
		*pDst = (flag < 0) ? tolower((int)*pSrc) :
				(flag > 0) ? toupper((int)*pSrc) : *pSrc ;
		++pDst ;
		++pSrc ;
	}
	*pDst = CHR_NUL ;									// terminate destination string
	return pSrc ;										// pointer to NULL or next char to be processed..
}

#define	delimDATE1	"-/"
#define	delimDATE2	"t "
#define	delimTIME1	"h:"
#define	delimTIME2	"m:"
#define	delimTIME3	"sz. "

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
char *	pcStringParseDateTime(char * pSrc, uint64_t * pTStamp, struct tm * psTM) {
	IF_myASSERT(debugPARAM, INRANGE_MEM(pSrc) && INRANGE_SRAM(pTStamp) && INRANGE_SRAM(psTM)) ;
	uint32_t	flag = 0 ;
	/* TPmax	= ThisPar max length+1
	 * TPact	= ThisPar actual length ( <0=error  0=not found  >0=length )
	 * NPact	= NextPar actual length
	 * TPlim	= ThisPar max value */
	int32_t		Value, TPmax, TPact, NPact, TPlim ;
	memset(psTM, 0, sizeof(struct tm)) ;				// ensure all start as 0
	while (*pSrc == CHR_SPACE) ++pSrc ;					// make sure no leading spaces ....

	IF_PRINT(debugPARSE_DTIME, "pcStringParseDateTime()\n") ;
	// check CCYY?MM? ahead
	TPact = xStringFindDelim(pSrc, delimDATE1, sizeof("CCYY")) ;
	NPact = (TPact > 0) ? xStringFindDelim(pSrc+TPact+1, delimDATE1, sizeof("MM")) : 0 ;
	IF_PRINT(debugPARSE_DTIME, "C: TPact=%d  NPact=%d", TPact, NPact) ;
	if (NPact >= 1) {
		IF_PRINT(debugPARSE_DTIME, "  Yr '%.*s'", TPact, pSrc) ;
		pSrc = pcStringParseValueRange(pSrc, (p32_t) &Value, vfIXX, vs32B, NULL, (x32_t) 0, (x32_t) YEAR_BASE_MAX) ;
		EQ_RETURN(pSrc, pcFAILURE) ;
		// Cater for CCYY vs YY form
		psTM->tm_year = INRANGE(YEAR_BASE_MIN, Value, YEAR_BASE_MAX, int32_t) ? Value - YEAR_BASE_MIN : Value ;
		flag |= DATETIME_YEAR_OK ;						// mark as done
		++pSrc ;										// skip over separator
	}
	IF_PRINT(debugPARSE_DTIME, "  Val=%d\n", psTM->tm_year) ;

	// check for MM?DD? ahead
	TPact = xStringFindDelim(pSrc, delimDATE1, sizeof("MM")) ;
	NPact = (TPact > 0) ? xStringFindDelim(pSrc+TPact+1, delimDATE2, sizeof("DD")) : 0 ;
	IF_PRINT(debugPARSE_DTIME, "M: TPact=%d  NPact=%d", TPact, NPact) ;

	if ((flag & DATETIME_YEAR_OK) || (NPact == 2) || (NPact == 0 && TPact > 0 && pSrc[TPact+3] == CHR_NUL)) {
		IF_PRINT(debugPARSE_DTIME, "  Mon '%.*s'", TPact, pSrc) ;
		pSrc = pcStringParseValueRange(pSrc, (p32_t) &Value, vfIXX, vs32B, NULL, (x32_t) 1, (x32_t) MONTHS_IN_YEAR) ;
		EQ_RETURN(pSrc, pcFAILURE) ;
		psTM->tm_mon = Value - 1 ;						// make 0 relative
		flag |= DATETIME_MON_OK ;						// mark as done
		++pSrc ;										// skip over separator
	}
	IF_PRINT(debugPARSE_DTIME, "  Val=%d\n", psTM->tm_mon) ;

	if (flag & (DATETIME_YEAR_OK | DATETIME_MON_OK)) {
		TPmax = sizeof("DD") ;
		TPlim = xTimeCalcDaysInMonth(psTM) ;
	} else {
		TPmax = sizeof("365") ;
		TPlim = DAYS_IN_YEAR ;
	}
	TPact = xStringFindDelim(pSrc, delimDATE2, TPmax) ;
	NPact = (TPact < 1 && pSrc[1] == CHR_NUL) ? 1 : (TPact < 1 && pSrc[2] == CHR_NUL) ? 2 : 0 ;
	IF_PRINT(debugPARSE_DTIME, "D: TPmax=%d  TPact=%d  NPact=%d  TPlim=%d", TPmax, TPact, NPact, TPlim) ;

	if ((flag & DATETIME_MON_OK) || (TPact > 0 && tolower((int) pSrc[TPact]) == CHR_t)) {
		IF_PRINT(debugPARSE_DTIME, "  Day '%.*s'", TPact, pSrc) ;
		pSrc = pcStringParseValueRange(pSrc, (p32_t) &Value, vfIXX, vs32B, NULL, (x32_t) 1, (x32_t) TPlim) ;
		EQ_RETURN(pSrc, pcFAILURE) ;
		psTM->tm_mday = Value ;
		flag |= DATETIME_MDAY_OK ;						// mark as done
	}
	IF_PRINT(debugPARSE_DTIME, "  Val=%d\n", psTM->tm_mday) ;

	// calculate day of year ONLY if yyyy-mm-dd read in...
	if (flag == (DATETIME_YEAR_OK | DATETIME_MON_OK | DATETIME_MDAY_OK)) {
		psTM->tm_yday = xTimeCalcDaysYTD(psTM) ;
		flag |= DATETIME_YDAY_OK ;
	}

	// skip over 'T' if there
	if (*pSrc == CHR_T || *pSrc == CHR_t || *pSrc == CHR_SPACE) {
		++pSrc ;
	}

	// check for HH?MM?
	if (flag & (DATETIME_YEAR_OK | DATETIME_MON_OK | DATETIME_MDAY_OK)) {
		TPmax = sizeof("HH") ;
		TPlim = HOURS_IN_DAY - 1 ;
	} else {
		TPmax = sizeof("8760") ;
		TPlim = HOURS_IN_YEAR ;
	}
	TPact = xStringFindDelim(pSrc, delimTIME1, TPmax) ;
	NPact = (TPact > 0) ? xStringFindDelim(pSrc+TPact+1, delimTIME2, sizeof("HH")) : 0 ;
	IF_PRINT(debugPARSE_DTIME, "H: TPmax=%d  TPact=%d  NPact=%d  TPlim=%d", TPmax, TPact, NPact, TPlim) ;

	if (NPact > 0) {
		IF_PRINT(debugPARSE_DTIME, "  Hr '%.*s'", TPact, pSrc) ;
		pSrc = pcStringParseValueRange(pSrc, (p32_t) &Value, vfIXX, vs32B, NULL, (x32_t) 0, (x32_t) TPlim) ;
		EQ_RETURN(pSrc, pcFAILURE) ;
		psTM->tm_hour = Value ;
		flag	|= DATETIME_HOUR_OK ;					// mark as done
		++pSrc ;										// skip over separator
	}
	IF_PRINT(debugPARSE_DTIME, "  Val=%d\n", psTM->tm_hour) ;

	// check for MM?SS?
	// [M..]M{m:}[S]S{s.Zz }
	if (flag & (DATETIME_YEAR_OK | DATETIME_MON_OK | DATETIME_MDAY_OK | DATETIME_HOUR_OK)) {
		TPmax = sizeof("MM") ;
		TPlim = MINUTES_IN_HOUR-1 ;
	} else {
		TPmax = sizeof("525600") ;
		TPlim = MINUTES_IN_YEAR ;
	}
	TPact = xStringFindDelim(pSrc, delimTIME2, TPmax) ;
	NPact = (TPact > 0) ? xStringFindDelim(pSrc+TPact+1, delimTIME3, sizeof("SS")) : 0 ;
	IF_PRINT(debugPARSE_DTIME, "M: TPmax=%d  TPact=%d  NPact=%d  TPlim=%d", TPmax, TPact, NPact, TPlim) ;

	if ((flag & DATETIME_HOUR_OK) || (NPact == 2) || (NPact == 0 && TPact > 0 && pSrc[TPact+3] == CHR_NUL)) {
		IF_PRINT(debugPARSE_DTIME, "  Min '%.*s'", TPact, pSrc) ;
		pSrc = pcStringParseValueRange(pSrc, (p32_t) &Value, vfIXX, vs32B, NULL, (x32_t) 0, (x32_t) TPlim) ;
		EQ_RETURN(pSrc, pcFAILURE) ;
		psTM->tm_min = Value ;
		flag	|= DATETIME_MIN_OK ;					// mark as done
		++pSrc ;										// skip over separator
	}
	IF_PRINT(debugPARSE_DTIME, "  Val=%d\n", psTM->tm_min) ;

	/*
	 * To support parsing of long (>60s) RELATIVE time period we must support values
	 * much bigger than 59.
	 * If NONE of the CCYY?MM?DD?HH?MM portions supplied then must be
	 *		SS[s.Zz ]
	 * else we must allow for
	 * 		SSSSSSSS[s.Zz ]
	 */
	if (flag & (DATETIME_YEAR_OK | DATETIME_MON_OK | DATETIME_MDAY_OK | DATETIME_HOUR_OK | DATETIME_MIN_OK)) {
		TPmax = sizeof("SS") ;
		TPlim = SECONDS_IN_MINUTE - 1 ;
	} else {
		TPmax = sizeof("31622399") ;
		TPlim = SECONDS_IN_LEAPYEAR - 1 ;
	}
	TPact = xStringFindDelim(pSrc, delimTIME3, TPmax) ;
	NPact = (TPact < 1) ? strlen(pSrc) : 0 ;
	IF_PRINT(debugPARSE_DTIME, "S: TPmax=%d  TPact=%d  NPact=%d  TPlim=%d", TPmax, TPact, NPact, TPlim) ;

	if ((flag & DATETIME_MIN_OK) || (TPact > 0) || (INRANGE(1, NPact, --TPmax, int32_t))) {
		IF_PRINT(debugPARSE_DTIME, "  Sec '%.*s'", TPact, pSrc) ;
		pSrc = pcStringParseValueRange(pSrc, (p32_t) &Value, vfIXX, vs32B, NULL, (x32_t) 0, (x32_t) TPlim) ;
		EQ_RETURN(pSrc, pcFAILURE) ;
		psTM->tm_sec = Value ;
		flag	|= DATETIME_SEC_OK ;					// mark as done
	}
	IF_PRINT(debugPARSE_DTIME, "  Val=%d\n", psTM->tm_sec) ;

	// check for [.0{...}Z] and skip as appropriate
	int32_t uSecs = 0 ;
	TPact = xStringFindDelim(pSrc, ".s", 1) ;
	TPmax = sizeof("999999") ;
	if (TPact == 0) {
		++pSrc ;										// skip over '.'
		TPact = xStringFindDelim(pSrc, "z ", TPmax) ;	// find position of Z/z in buffer
		if (OUTSIDE(1, TPact, TPmax, int32_t)) {
		/* XXX valid terminator not found, but maybe a NUL ?
		 * still a problem, what about junk after the last number ? */
			NPact = strlen(pSrc) ;
			if (OUTSIDE(1, NPact, --TPmax , int32_t)) {
				return pcFAILURE ;
			}
			TPact = NPact ;
		}
		IF_PRINT(debugPARSE_DTIME, " uS '%.*s'", TPact, pSrc) ;
		pSrc = pcStringParseValueRange(pSrc, (p32_t) &uSecs, vfIXX, vs32B, NULL, (x32_t) 0, (x32_t) (MICROS_IN_SECOND-1)) ;
		EQ_RETURN(pSrc, pcFAILURE) ;
		TPact = 6 - TPact ;
		while (TPact--) {
			uSecs *= 10 ;
		}
		flag |= DATETIME_MSEC_OK ;						// mark as done
		IF_PRINT(debugPARSE_DTIME, "  Val=%d\n", uSecs) ;
	}

	if (pSrc[0]==CHR_Z || pSrc[0]==CHR_z) {
		++pSrc ;						// skip over trailing 'Z'
	}

	uint32_t Secs ;
	if (flag & DATETIME_YEAR_OK) {						// full timestamp data found?
		psTM->tm_wday = (xTimeCalcDaysToDate(psTM) + timeEPOCH_DAY_0_NUM) % DAYS_IN_WEEK ;
		psTM->tm_yday = xTimeCalcDaysYTD(psTM) ;
		Secs = xTimeCalcSeconds(psTM, 0) ;
	} else {
		Secs = xTimeCalcSeconds(psTM, 1) ;
	}
	*pTStamp = xTimeMakeTimestamp(Secs, uSecs) ;
	IF_PRINT(debugPARSE_DTIME, "flag=%p  uS=%'llu  wday=%d  yday=%d  %04d/%02d/%02d  %dh%02dm%02ds\n",
								flag, *pTStamp, psTM->tm_wday, psTM->tm_yday,
								psTM->tm_year, psTM->tm_mon, psTM->tm_mday,
								psTM->tm_hour, psTM->tm_min, psTM->tm_sec) ;
	return pSrc ;
}

/**
 * pcCodeToMessage() - locates the provided eCode within the table specified and return the message
 * @param[in]	eCode - (error) code to lookup
 * @param[in]	eTable - pointer to message table within which to look
 * @return		pointer to correct/matched or default (not found) message
 */
char * pcCodeToMessage(int32_t eCode, const eTable_t * eTable) {
	int32_t	i = 0 ;
	while ((eTable[i].uVal1 != 0xFFFFFFFF) && (eTable[i].uVal2 != 0xFFFFFFFF)) {
	// If an exact match with 1st value in table, return pointer to the message
		if (eCode == eTable[i].iVal1) {
			break ;
		}
	// If we have a 2nd (ie range) value, then test differently for positive & negative ranges
		if (eTable[i].iVal2 != 0) {
		// code is negative (ie an error code) and within this (negative) range, return pointer to message
			if ((eCode < 0) && (eCode < eTable[i].iVal1) && (eCode >= eTable[i].iVal2)) {
				break ;
			}
		// if code is positive (ie a normal) and within the positive range, return pointer to message
			if ((eCode > 0) && (eCode > eTable[i].iVal1) && (eCode <= eTable[i].iVal2)) {
				break ;
			}
		}
		++i ;											// no match found, try next entry
	}
	return (char *) eTable[i].pMess ;
}

// ############################## Bitmap to string decode functions ################################

int32_t	xBitMapDecodeChanges(uint32_t Val1, uint32_t Val2, uint32_t Mask, const char * pMesArray[], char * pBuf, size_t BufSize) {
	int32_t	pos, idx, BufLen = 0 ;
	uint32_t	CurMask, ColCode ;
	for (pos = 31, idx = 31, CurMask = 0x80000000 ; pos >= 0; CurMask >>= 1, --pos, --idx) {
		if (Mask & CurMask) {
			if ((Val1 & CurMask) && (Val2 & CurMask)) {
				ColCode = xpfSGR(attrRESET, colourFG_WHITE, 0, 0) ;
			} else if (Val1 & CurMask) {
				ColCode = xpfSGR(attrRESET, colourFG_RED, 0, 0) ;
			} else if (Val2 & CurMask) {
				ColCode = xpfSGR(attrRESET, colourFG_GREEN, 0, 0) ;
			} else {
				ColCode = 0 ;
			}
			if (ColCode) {
				BufLen += snprintfx(pBuf + BufLen, BufSize - BufLen, "  %C%s%C", ColCode, pMesArray[idx], attrRESET) ;
			}
		}
	}
	return BufLen ;
}

int32_t	xBitMapDecode(uint32_t Value, uint32_t Mask, const char * pMesArray[], char * pBuf, size_t BufSize) {
	int32_t	pos, idx, BufLen = 0 ;
	uint32_t	CurMask ;
	for (pos = 31, idx = 0, CurMask = 0x80000000 ; pos >= 0; CurMask >>= 1, pos--) {
		if (Mask & CurMask) {
			if (Value & CurMask) {
				BufLen += snprintf(pBuf + BufLen, BufSize - BufLen, "  %s", pMesArray[idx]) ;
			}
			idx++ ;
		}
	}
	return BufLen ;
}

/**
 * vBitMapDecode() - decodes a bitmapped value using a bit-mapped mask
 * @brief
 * @param	Value - 32bit value to the decoded
 * @param	Mask - 32bit mask to be applied
 * @param	pMesArray - pointer to array of messages (1 per bit SET in the mask)
 * return	none
 */
void	vBitMapDecode(uint32_t Value, uint32_t Mask, const char * pMesArray[]) {
	int32_t	pos, idx ;
	uint32_t	CurMask ;
	if (Mask) {
		for (pos = 31, idx = 0, CurMask = 0x80000000 ; pos >= 0; CurMask >>= 1, --pos) {
			if (CurMask & Mask & Value) {
				PRINT(" |%02d|%s|", pos, pMesArray[idx]) ;
			}
			if (CurMask & Mask) {
				idx++ ;
			}
		}
	}
}

/**
 * vBitMapReport() - decodes a bitmapped value using a bitmapped mask
 * @brief	if pName is 0, the hex value will not be displayed and no postpend CR/LF will be added
 * @param	pName - Pointer to the name/label prefixed to the decoded starting output
 * @param	Value - 32bit value to the decoded
 * @param	Mask - 32bit mask to be applied
 * @param	pMesArray - pointer to array of messages (1 per bit SET in the mask)
 * return	none
 */
void	vBitMapReport(char * pName, uint32_t Value, uint32_t Mask, const char * pMesArray[]) {
	IF_myASSERT(debugPARAM, INRANGE_MEM(pMesArray)) ;
	if (pName != NULL) {
		PRINT(" %s 0x%02x:", pName, Value) ;
	}
	vBitMapDecode(Value, Mask, pMesArray) ;
	if (pName != NULL) {
		PRINT("\n") ;
	}
}

/**
 * xStringValueMap() - build an output string using bit-mapped mask to select characters from a source string
 * @brief	with source string "ABCDEFGHIJKLMNOPQRST" and value 0x000AAAAA will build "A-C-E-G-I-K-M-O-Q-S-"
 * @param	pString
 * @param	pBuf
 * @param	uValue
 * @param	iWidth
 * @return
 */
int32_t	xStringValueMap(const char * pString, char * pBuf, uint32_t uValue, int32_t iWidth) {
	IF_myASSERT(debugPARAM, INRANGE_FLASH(pString) && INRANGE_SRAM(pBuf) && (iWidth <= 32) && (strnlen(pString, 33) <= iWidth)) ;
	uint32_t uMask = 0x8000 >> (32 - iWidth) ;
	int32_t Idx ;
	for (Idx = 0; Idx < iWidth; ++Idx, ++pString, ++pBuf, uMask >>= 1) {
		*pBuf = (uValue | uMask) ? *pString : CHR_MINUS ;
	}
	*pBuf = CHR_NUL ;
	return Idx ;
}

// #################################################################################################

#define		stringTEST_FLAG			0x0000

#define		stringTEST_EPOCH		(stringTEST_FLAG & 0x0001)
#define		stringTEST_DATES		(stringTEST_FLAG & 0x0002)
#define		stringTEST_TIMES		(stringTEST_FLAG & 0x0004)
#define		stringTEST_DTIME		(stringTEST_FLAG & 0x0008)

#define		stringTEST_RELDAT		(stringTEST_FLAG & 0x0010)

void x_string_general_test(void) {
#if		(stringTEST_FLAG)
	struct	tm	sTM ;
	TSZ_t	sTSZ ;
#endif

#if		(stringTEST_EPOCH)
	char	test[64] ;
	sTSZ.usecs	= xTimeMakeTimestamp(SECONDS_IN_EPOCH_PAST, 0) ;
	xsnprintf(test, sizeof(test), "%Z", &sTSZ) ;
	pcStringParseDateTime(test, &sTSZ.usecs, &sTM) ;
	PRINT("Input: %s => Date: %s %02d %s %04d Time: %02d:%02d:%02d Day# %d\n",
			test, xTime_GetDayName(sTM.tm_wday), sTM.tm_mday, xTime_GetMonthName(sTM.tm_mon), sTM.tm_year + YEAR_BASE_MIN,
			sTM.tm_hour, sTM.tm_min, sTM.tm_sec, sTM.tm_yday) ;

	sTSZ.usecs	= xTimeMakeTimestamp(SECONDS_IN_EPOCH_FUTURE, 999999) ;
	xsnprintf(test, sizeof(test), "%Z", &sTSZ) ;
	pcStringParseDateTime(test, &sTSZ.usecs, &sTM) ;
	PRINT("Input: %s => Date: %s %02d %s %04d Time: %02d:%02d:%02d Day# %d\n",
			test, xTime_GetDayName(sTM.tm_wday), sTM.tm_mday, xTime_GetMonthName(sTM.tm_mon), sTM.tm_year + YEAR_BASE_MIN,
			sTM.tm_hour, sTM.tm_min, sTM.tm_sec, sTM.tm_yday) ;
#endif

#if		(stringTEST_DATES)
	pcStringParseDateTime((char *) "2019/04/15", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "2019/04/15 ", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "2019/04/15T", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "2019/04/15t", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;

	pcStringParseDateTime((char *) "2019-04-15", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "2019-04-15 ", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "2019-04-15T", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "2019-04-15t", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
#endif

#if		(stringTEST_TIMES)
	pcStringParseDateTime((char *) "1", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "1 ", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "01", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "01Z", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "1z", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;

	pcStringParseDateTime((char *) "1.1", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1100000 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "1.1 ", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1100000 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "1.1Z", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1100000 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "1.1z", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1100000 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;

	pcStringParseDateTime((char *) "1.000001", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1000001 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "1.000001 ", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1000001 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "1.000001Z", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1000001 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "1.000001z", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1000001 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
#endif

#if		(stringTEST_DTIME)
	pcStringParseDateTime((char *) "2019/04/15T1:23:45.678901Z", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=1 || sTM.tm_min!=23 || sTM.tm_sec!=45 || (sTSZ.usecs % MILLION) != 678901 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;

	pcStringParseDateTime((char *) "2019-04-15t01h23m45s678901z", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=1 || sTM.tm_min!=23 || sTM.tm_sec!=45 || (sTSZ.usecs % MILLION) != 678901 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;

	pcStringParseDateTime((char *) "2019/04-15 1:23m45.678901", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=1 || sTM.tm_min!=23 || sTM.tm_sec!=45 || (sTSZ.usecs % MILLION) != 678901 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;

	pcStringParseDateTime((char *) "2019-04/15t01h23:45s678901", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=1 || sTM.tm_min!=23 || sTM.tm_sec!=45 || (sTSZ.usecs % MILLION) != 678901 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
#endif

#if		(stringTEST_RELDAT)
	pcStringParseDateTime((char *) "1/1-1", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=1 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
	pcStringParseDateTime((char *) "1-1/1", &sTSZ.usecs, &sTM) ;
	PRINT(sTM.tm_year!=1 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\n" : " #%d Passed\n", __LINE__) ;
#endif
}
