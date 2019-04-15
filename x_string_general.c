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

#include	"x_debug.h"
#include	"x_string_general.h"
#include	"x_string_to_values.h"
#include	"x_errors_events.h"
#include	"x_syslog.h"

#include	<string.h>
#include	<ctype.h>

#define	debugFLAG						0xC040

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

/**
 * xstrrev
 * @param  str is a pointer to the string to be reversed
 * @return none
 * @brief  perform 'in-place' start <-> end charater reversal
 */
void	xstrrev(char * pStr) {
	IF_myASSERT(debugPARAM, INRANGE_SRAM(pStr)) ;
	if ((pStr == NULL) || (*pStr == CHR_NUL)) {
		return ;
	}
	char *p1, *p2 ;
	for (p1 = pStr, p2 = pStr + xstrlen(pStr) - 1; p2 > p1; ++p1, --p2) {
		*p1 ^= *p2 ;
		*p2 ^= *p1 ;
		*p1 ^= *p2 ;
	}
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
	IF_myASSERT(debugPARAM, INRANGE_MEM(s1) && INRANGE_MEM(s2) && (xLen < 1024)) ;
	IF_SL_DBG(debugXSTRCMP, " S1=%s:S2=%s ", s1, s2) ;
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
	int32_t iRetVal = 0 ;
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
		++iRetVal ;										// & adjust count...
	}
	*pDst = CHR_NUL ;									// terminate
	IF_PRINT(debugPARSE_ENCODED, "%s\n", pStr-iRetVal) ;
	return iRetVal ;
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
	IF_myASSERT(debugPARAM, INRANGE_SRAM(pDst) && INRANGE_MEM(pSrc) && INRANGE_MEM(pDel)) ;
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

 * pcStringParseDateTime()
 * @brief		parse a string with format	2015-04-01T12:34:56.789Z or
 * 											2015/04/01T23:34:45.678Z or
 * 											2015/04/01T23h12m54s123Z
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
char *	pcStringParseDateTime(char * pBuf, uint64_t * pTStamp, struct tm * psTM) {
	IF_myASSERT(debugPARAM, INRANGE_MEM(psTM) && INRANGE_SRAM(pTStamp) && INRANGE_SRAM(psTM)) ;
	memset(psTM, 0, sizeof(struct tm)) ;				// ensure all start as 0
// make sure no leading spaces ....
	while (*pBuf == CHR_SPACE) {
		pBuf++ ;
	}
	int32_t xLen = strlen((const char *)pBuf) ;					// and then get length of remainder...

// what about specifying [2]101-MM-DD and nothing else ?
	int32_t idx = 	((pBuf[4] == CHR_MINUS) || (pBuf[4] == CHR_FWDSLASH)) ? 4 :
					((pBuf[3] == CHR_MINUS) || (pBuf[3] == CHR_FWDSLASH)) ? 3 :
					((pBuf[2] == CHR_MINUS) || (pBuf[2] == CHR_FWDSLASH)) ? 2 :
					((pBuf[1] == CHR_MINUS) || (pBuf[1] == CHR_FWDSLASH)) ? 1 : 0 ;

	IF_SL_DBG(debugPARSE_DTIME, "[CCYY?] Parsing '%s'", pBuf) ;
	// check CCYY-MM-DD ahead
	uint32_t	flag = 0 ;
	int32_t	Value ;
	if ((xLen >= (idx + 6)) &&
		(pBuf[idx]   == CHR_MINUS	|| pBuf[idx]	== CHR_FWDSLASH) &&
		(pBuf[idx+3] == CHR_MINUS	|| pBuf[idx+3]	== CHR_FWDSLASH)) {
		pBuf = pcStringParseNumberRange(&Value, pBuf, 0, YEAR_BASE_MAX) ;
		CHECK_RETURN(pBuf, pcFAILURE) ;
		if (INRANGE(YEAR_BASE_MIN, Value, YEAR_BASE_MAX, int32_t)) {
			psTM->tm_year = Value - YEAR_BASE_MIN ;
			flag |= DATETIME_YEAR_OK ;					// mark as done
		} else {
			psTM->tm_year = Value ;						// save as elapsed
		}
		xLen -= (idx + 1) ;
		pBuf++ ;										// skip over separator
		idx = 2 ;
	} else {
		idx = 0 ;
	}
// what about specifying [M]M-DD and nothing else ?
	if (idx < 2) {
		idx =	(pBuf[2] == CHR_MINUS  || pBuf[2] == CHR_FWDSLASH) &&
				(pBuf[5] == CHR_T || pBuf[5] == CHR_t || pBuf[5] == CHR_NUL) ? 2 : 1 ;
	}

// check for MM-DD ahead
	IF_SL_DBG(debugPARSE_DTIME, "[MM?DD?] Parsing '%s'", pBuf) ;
	if ((xLen >= (idx + 3)) &&
		(pBuf[2] == CHR_MINUS || pBuf[2] == CHR_FWDSLASH) &&
		(pBuf[5] == CHR_T || pBuf[5] == CHR_t || pBuf[5] == CHR_NUL)) {
		pBuf = pcStringParseNumberRange(&Value, pBuf, 1, MONTHS_IN_YEAR) ;
		CHECK_RETURN(pBuf, pcFAILURE) ;
		psTM->tm_mon = Value - 1 ;						// make 0 relative
		flag |= DATETIME_MON_OK ;						// mark as done
		xLen -= (idx + 1) ;
		pBuf++ ;										// skip over separator
	}

// what about specifying [D]D and nothing else ?
	if (idx < 2) {
		idx = (pBuf[2] == CHR_T || pBuf[2] == CHR_t  || pBuf[2] == CHR_NUL) ? 2 : 1 ;
	}

// check for DD[T] ahead
	IF_SL_DBG(debugPARSE_DTIME, "[DD?] Parsing '%s'", pBuf) ;
	if ((xLen >= idx) && (pBuf[2] == CHR_T || pBuf[2] == CHR_t || pBuf[2] == CHR_NUL)) {
		pBuf = pcStringParseNumberRange(&Value, pBuf, 1, xTime_CalcDaysInMonth(psTM)) ;
		CHECK_RETURN(pBuf, pcFAILURE) ;
		psTM->tm_mday = Value ;
		flag |= DATETIME_MDAY_OK ;						// mark as done
		xLen -= idx ;
	}

// calculate day of year ONLY if yyyy-mm-dd read in...
	if (flag == (DATETIME_YEAR_OK | DATETIME_MON_OK | DATETIME_MDAY_OK)) {
		psTM->tm_yday = xTime_CalcDaysYTD(psTM) ;
		flag |= DATETIME_YDAY_OK ;
	}
// skip over 'T' if there
	if ((*pBuf == CHR_T) || (*pBuf == CHR_t)) {
		pBuf++ ;
		xLen-- ;
	}

// what about specifying [0]9:59:59 and nothing else ?
	if (idx < 2) {
		idx = ((pBuf[2] == CHR_COLON) && (pBuf[5] == CHR_COLON)) ? 2 : 1 ;
	}

// check for hh:mm:ss
	IF_SL_DBG(debugPARSE_DTIME, "[HH?MM?SS?] Parsing '%s'", pBuf) ;
	if ((xLen >= (idx + 6)) && pBuf[idx] == CHR_COLON && pBuf[idx+3] == CHR_COLON) {
		pBuf = pcStringParseNumberRange(&Value, pBuf, 0, HOURS_IN_DAY - 1) ;
		CHECK_RETURN(pBuf, pcFAILURE) ;
		psTM->tm_hour = Value ;
		flag	|= DATETIME_HOUR_OK ;					// mark as done
		xLen	-= (idx + 1) ;
		idx		= 2 ;
		pBuf++ ;										// skip over separator
	}

// what about specifying [0]9:59 and nothing else ?
	if (idx < 2) {
		idx = ((pBuf[2] == CHR_COLON) &&
				(pBuf[5] == CHR_Z || pBuf[5] == CHR_z || pBuf[5] == CHR_FULLSTOP || pBuf[5] == CHR_SPACE || pBuf[5] == CHR_NUL)) ? 2 : 1 ;
	}

// check for mm:ss
	IF_SL_DBG(debugPARSE_DTIME, "[MM?SS?] Parsing '%s'", pBuf) ;
	if ((xLen >= (idx+3)) && (pBuf[idx] == CHR_COLON &&
		(pBuf[idx+3] == CHR_Z || pBuf[idx+3] == CHR_z || pBuf[idx+3] == CHR_FULLSTOP || pBuf[idx+3]==CHR_SPACE || pBuf[idx+3] == CHR_NUL))) {
		pBuf = pcStringParseNumberRange(&Value, pBuf, 0, MINUTES_IN_HOUR - 1) ;
		CHECK_RETURN(pBuf, pcFAILURE) ;
		psTM->tm_min = Value ;
		flag	|= DATETIME_MIN_OK ;					// mark as done
		xLen	-= (idx + 1) ;
		idx		= 2 ;
		pBuf++ ;										// skip over separator
	}

// what about specifying [0]9 and nothing else ?
	if (idx < 2) {
		idx = (pBuf[2]==CHR_FULLSTOP || pBuf[2]==CHR_Z || pBuf[2]==CHR_z || pBuf[2]==CHR_SPACE || pBuf[2]==CHR_NUL) ? 2 : 1 ;
	}

// check for ss[. Z]
	IF_SL_DBG(debugPARSE_DTIME, "[SS?] Parsing '%s'", pBuf) ;
	if ((xLen >= idx) &&
		(pBuf[idx] == CHR_FULLSTOP || pBuf[idx] == CHR_Z || pBuf[idx] == CHR_z || pBuf[idx] == CHR_SPACE || pBuf[idx] == CHR_NUL)) {
		pBuf = pcStringParseNumberRange(&Value, pBuf, 0, SECONDS_IN_MINUTE - 1) ;
		CHECK_RETURN(pBuf, pcFAILURE) ;
		psTM->tm_sec = Value ;
		flag	|= DATETIME_SEC_OK ;					// mark as done
		xLen	-= (idx + 1) ;
	}

// check for [.0{...}Z] and skip as appropriate
	int32_t count, uSecs = 0 ;
	if ((xLen >= 1) && (pBuf[0] == CHR_FULLSTOP)) {
		pBuf++ ;										// skip over '.'
		count = xinstring(pBuf, CHR_Z) ;				// find position of 'Z' in buffer
		if (count == erFAILURE || count > 6) {
			count = xinstring(pBuf, CHR_z) ;			// try again for 'z' in buffer
			if (count == erFAILURE || count > 6) {
				return pcFAILURE ;
			}
		}
		IF_SL_DBG(debugPARSE_DTIME, "[.1-6D?] Parsing '%s'", pBuf) ;
		pBuf = pcStringParseNumberRange((int32_t *) &uSecs, pBuf, 0, MICROS_IN_SECOND - 1) ;
		CHECK_RETURN(pBuf, pcFAILURE) ;
		count = 6 - count ;
		while (count--) {
			uSecs *= 10 ;
		}
		flag |= DATETIME_MSEC_OK ;						// mark as done
	}
	if (pBuf[0]==CHR_Z || pBuf[0]==CHR_z) {
		pBuf++ ;						// skip over trailing 'Z'
	}

	uint32_t Secs ;
	if (flag & DATETIME_YEAR_OK) {						// full timestamp data found?
		psTM->tm_wday = (xTime_CalcDaysToDate(psTM) + timeEPOCH_DAY_0_NUM) % DAYS_IN_WEEK ;
		psTM->tm_yday = xTime_CalcDaysYTD(psTM) ;
		Secs = xTime_CalcSeconds(psTM, 0) ;
	} else {
		Secs = xTime_CalcSeconds(psTM, 1) ;
	}
	IF_SL_DBG(debugPARSE_DTIME, "flag=%p %u.%u wday=%d yday=%d %d/%02d/%02d %dh%02dm%02ds",
						flag, Secs, uSecs, psTM->tm_wday, psTM->tm_yday,
						psTM->tm_year, psTM->tm_mon, psTM->tm_mday, psTM->tm_hour, psTM->tm_min, psTM->tm_sec) ;
	*pTStamp = xTimeMakeTimestamp(Secs, uSecs) ;
	return pBuf ;
}

/*
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
		i++ ;											// no match found, try next entry
	}
	return (char *) eTable[i].pMess ;
}

// ############################## Bitmap to string decode functions ################################

int32_t	xBitMapDecode(uint32_t Value, uint32_t Mask, const char * pMesArray[], char * pBuf, size_t BufSize) {
	int32_t	pos, idx, BufLen = 0 ;
	uint32_t	CurMask ;
	for (pos = 31, idx = 0, CurMask = 0x80000000 ; pos >= 0; CurMask >>= 1, pos--) {
		if (Mask & CurMask) {
			if (Value & CurMask) {
				BufLen += snprintf(pBuf + BufLen, BufSize - BufLen, " %02d=%s", pos, pMesArray[idx]) ;
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

void x_string_general_test(void) {
	TSZ_t	sTSZ ;
	sTSZ.usecs	= xTimeMakeTimestamp(SECONDS_IN_EPOCH_PAST, 0) ;
	struct	tm	sTM ;
	char test[64] ;
	xsnprintf(test, sizeof(test), "%Z", &sTSZ) ;
	pcStringParseDateTime(test, &sTSZ.usecs, &sTM) ;
	PRINT("Input: %s => Date: %s %02d %s %04d Time: %02d:%02d:%02d Day# %d\n",
		test, xTime_GetDayName(sTM.tm_wday), sTM.tm_mday, xTime_GetMonthName(sTM.tm_mon), sTM.tm_year + YEAR_BASE_MIN,
		sTM.tm_hour, sTM.tm_min, sTM.tm_sec, sTM.tm_yday) ;

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
}
