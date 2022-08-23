/*
 * x_string_general.c
 * Copyright (c) 2014-22 Andre M. Maree / KSS Technologies (Pty) Ltd.
 */

#include "hal_variables.h"

#include "x_string_general.h"
#include "x_string_to_values.h"
#include "FreeRTOS_Support.h"
#include "printfx.h"									// +x_definitions +stdarg +stdint +stdio
#include "syslog.h"
#include "options.h"

#include "x_errors_events.h"
#include "x_time.h"

#define	debugFLAG					0xF000

#define	debugXSTRCMP				(debugFLAG & 0x0001)
#define	debugPARSE_U64				(debugFLAG & 0x0002)
#define	debugPARSE_F64				(debugFLAG & 0x0004)
#define	debugPARSE_X64				(debugFLAG & 0x0008)

#define	debugPARSE_ENCODED			(debugFLAG & 0x0100)
#define	debugDELIM					(debugFLAG & 0x0200)

#define	debugTIMING					(debugFLAG_GLOBAL & debugFLAG & 0x1000)
#define	debugTRACK					(debugFLAG_GLOBAL & debugFLAG & 0x2000)
#define	debugPARAM					(debugFLAG_GLOBAL & debugFLAG & 0x4000)
#define	debugRESULT					(debugFLAG_GLOBAL & debugFLAG & 0x8000)

#ifndef stringMAX_LEN
	#define	stringMAX_LEN			2048
#endif

#define	controlSIZE_FLAGS_BUF		(24 * 10)

#define	delimDATE1	"-/"
#define	delimDATE2	"t "
#define	delimTIME1	"h:"
#define	delimTIME2	"m:"
#define	delimTIME3	"sz. "

/**
 * @brief	verify to a maximum number of characters that each character is within a range
 * @return	erSUCCESS if cNum or fewer chars tested OK and a NUL is reached
 */
int	xstrverify(char * pStr, char cMin, char cMax, char cNum) {
	if (*pStr == 0)
		return erFAILURE;
	while (cNum--) {
		if (OUTSIDE(cMin, *pStr, cMax))
			return erFAILURE;
	}
	return erSUCCESS;
}

/**
 * @brief	calculate the length of the string up to max len specified
 * @param	s		pointer to the string
 * @param	len		maximum length to check for/return
 * @return	length of the string excl the terminating '\0'
 */
int	xstrnlen(const char * s, int len) {
	int l ;
	for (l = 0; *s != 0 && l < len; ++s, ++l) ;
	return l ;
}

/**
 * @brief	Copies from s2 to s1 until either 'n' chars copied or '\000' reached in s2
 * 			String will ONLY be terminated if less than maximum characters copied.
 * @param	s1 - pointer to destination uint8_t array (string)
 *			s2 - pointer to source uint8_t array (string)
 *			n - maximum number of chars to copy
 * @return	Actual number of chars copied (x <= n) excluding possible NULL
 */
int	xstrncpy(char * pDst, char * pSrc, int xLen ) {
	IF_myASSERT(debugPARAM, halCONFIG_inSRAM(pDst) && halCONFIG_inMEM(pSrc) && xLen) ;
	int Cnt = 0 ;
	while (*pSrc != 0 && Cnt < xLen) {
		*pDst++ = *pSrc++ ;								// copy across and adjust both pointers
		Cnt++ ;											// adjust length copied
	}
	if (Cnt < xLen)
		*pDst = 0;										// if space left, terminate
	return Cnt ;
}

int	xmemrev(char * pMem, size_t Size) {
	IF_myASSERT(debugPARAM, halCONFIG_inSRAM(pMem) && Size > 1) ;
	if (pMem == NULL || *pMem == 0 || Size < 2)
		return erFAILURE;
	char * pRev = pMem + Size - 1 ;
	#if (stringXMEMREV_XOR == 1)
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
 * @param  str is a pointer to the string to be reversed
 * @return none
 * @brief  perform 'in-place' start <-> end character reversal
 */
void xstrrev(char * pStr) { xmemrev(pStr, strlen(pStr)); }

/**
 * @brief	determine position of character in string (if at all)
 * @param		pStr - pointer to string to scan
 * 				cChr - the character match to find
 * @return		0 -> n the index into the string where the char is found
 * 				FAILURE if no match found, or cChr is NULL
 */
int	strchr_i(const char * pStr, char cChr) {
	IF_myASSERT(debugPARAM, halCONFIG_inMEM(pStr));
	char * pTmp = strchr(pStr, cChr);
	return (pTmp != NULL) ? (pTmp - pStr) : erFAILURE;
}

/**
 * @brief	based on flag case in/sensitive
 * @param	s1/2 - pointers to strings to be compared
 * @param	xLen - maximum length to compare (non null terminated string)
 * @param	flag - true for exact match, else upper/lower case difference ignored
 * @return			true or false based on comparison
 */
int	xstrncmp(const char * s1, const char * s2, size_t xLen, bool Exact) {
	IF_myASSERT(debugPARAM, halCONFIG_inMEM(s1) && halCONFIG_inMEM(s2) && xLen < 1024) ;
	while (*s1 && *s2 && xLen) {
		if (Exact == true) {
			if (*s1 != *s2)
				break;
		} else {
			if (toupper((int)*s1) != toupper((int)*s2))
				break;
		}
		++s1 ;
		++s2 ;
		--xLen ;
	}
	return (*s1 == 0 && *s2 == 0) ? 1 : (xLen == 0) ? 1 : 0 ;
}

/**
 * @brief	compare two strings based on flag case in/sensitive
 * @param	s1, s2 - pointers to strings to be compared
 * 			flag - true for exact match, else upper/lower case difference ignored
 * @return	true or false based on comparison
 */
int	xstrcmp(const char * s1, const char * s2, bool Exact) {
	IF_myASSERT(debugPARAM, halCONFIG_inMEM(s1) && halCONFIG_inMEM(s2)) ;
	while (*s1 && *s2) {
		if (Exact) {
			if (*s1 != *s2)
				break;
		} else {
			if (toupper((int)*s1) != toupper((int)*s2))
				break;
		}
		++s1 ;
		++s2 ;
	}
	return (*s1 == 0 && *s2 == 0) ? 1 : 0 ;
}

/**
 * @brief	determine array index of specified string in specified string array
 * 			expects the array to be null terminated
 * @param	key	- pointer to key string to find
 * 			table - pointer to array of pointers to search in.
 * 			flag - true for exact match, else upper/lower case difference ignored
 * @return	if match found, index into array else FAILURE
 */
int	xstrindex(char * key, char * array[]) {
	int	i = 0 ;
	while (array[i]) {
		if (strcasecmp(key, array[i]) == 0)
			return i;									// strings match, return index
		++i ;
	}
	return erFAILURE ;
}

// ############################### string parsing routines #########################################

/**
 * @brief	parse an encoded string (optionally in-place) to a non-encoded string
 * @return	erFAILURE if illegal/unexpected characters encountered
 * 			erSUCCESS if a zero length string encountered
 * 			1 or greater = length of the parsed string
 */
int	xStringParseEncoded(char * pDst, char * pSrc) {
	IF_myASSERT(debugPARAM, halCONFIG_inSRAM(pSrc)) ;
	IF_myASSERT(debugPARAM && (pDst != NULL), halCONFIG_inSRAM(pDst));
	int iRV = 0 ;
	if (pDst == NULL) {
		pDst = pSrc ;
	}
	IF_P(debugPARSE_ENCODED, "%s  ", pSrc) ;
	while(*pSrc != 0) {
		if (*pSrc == CHR_PERCENT) {						// escape char?
			int Val1 = xHexCharToValue(*++pSrc, BASE16) ;	// yes, parse 1st value
			if (Val1 == erFAILURE)
				return erFAILURE;
			int Val2 = xHexCharToValue(*++pSrc, BASE16) ;	// parse 2nd value
			if (Val2 == erFAILURE)
				return erFAILURE ;
			IF_P(debugPARSE_ENCODED, "[%d+%d=%d]  ", Val1, Val2, (Val1 << 4) + Val2) ;
			*pDst++ = (Val1 << 4) + Val2 ;				// calc & store final value
			++pSrc ;									// step to next char
		} else {
			*pDst++ = *pSrc++ ;							// copy as is to (new) position
		}
		++iRV ;											// & adjust count...
	}
	*pDst = 0;
	IF_P(debugPARSE_ENCODED, "%s\r\n", pSrc-iRV) ;
	return iRV ;
}

int	xStringParseUnicode(char * pDst, char * pSrc, size_t Len) {
	IF_myASSERT(debugPARAM, halCONFIG_inSRAM(pSrc));
	IF_myASSERT(debugPARAM && (pDst != NULL), halCONFIG_inSRAM(pDst));
	int iRV = 0;
	if (pDst == NULL) {
		pDst = pSrc;
	}
	IF_RP(debugPARSE_ENCODED, "%s  ", pSrc);
	while(*pSrc != 0 && (iRV < Len)) {
		if (*pSrc == CHR_BACKSLASH && *(pSrc+1) == CHR_u) {		// escape chars?
			int Val = 0;
			for (int i = 2; i < 6; ++i) {
				int iRV2 = xHexCharToValue(pSrc[i], BASE16);
				IF_RP(debugPARSE_ENCODED, "%c/%d/%d  ",pSrc[i], iRV2, Val);
				if (iRV2 == erFAILURE)
					return iRV2;
				Val <<= 4;
				Val += iRV2;
			}
			if (Val > 255) {
				*pDst++ = Val >> 8;
				++iRV;
			}
			*pDst++ = Val & 0xFF;
			pSrc += 6;
			IF_RP(debugPARSE_ENCODED, "%d  ", Val);
		} else {
			*pDst++ = *pSrc++;							// copy as is to (new) position
		}
		++iRV;											// & adjust count...
	}
	if (iRV < Len)
		*pDst = 0;
	IF_RP(debugPARSE_ENCODED, "%.*s\r\n", iRV, pDst-iRV);
	return iRV;
}

/**
 * @brief	skips specified delimiters (if any)
 * @brief	does NOT automatically work on assumption that string is NULL terminated, hence requires MaxLen
 * @brief	ONLY if MaxLen specified as NULL, assume string is terminated and calculate length.
 * @param[in]	pSrc - pointer to source buffer
 * @param[in]	pDel - pointer to string of valid delimiters
 * @param[in]	MaxLen - maximum number of characters in buffer
 * @return		number of delimiters (to be) skipped
 */
int	xStringSkipDelim(char * pSrc, const char * pDel, size_t MaxLen) {
	IF_myASSERT(debugPARAM, halCONFIG_inMEM(pSrc) && halCONFIG_inMEM(pDel));
	// If no length supplied
	if (MaxLen == 0) {
		MaxLen = xstrnlen(pSrc, stringMAX_LEN);	// assume NULL terminated and calculate length
		IF_myASSERT(debugRESULT, MaxLen < stringMAX_LEN) ;		// just a check to verify not understated
	}
	IF_P(debugDELIM, " '%.4s'", pSrc) ;
	// continue skipping over valid terminator characters
	int	CurLen = 0 ;
	while (strchr(pDel, *pSrc) && (CurLen < MaxLen)) {
		++pSrc ;
		++CurLen ;
	}
	IF_P(debugDELIM, "->'%.4s'", pSrc) ;
	return CurLen ;								// number of delimiters skipped over
}

int	xStringFindDelim(char * pSrc, const char * pDlm, size_t xMax) {
	IF_myASSERT(debugPARAM, halCONFIG_inMEM(pSrc) && halCONFIG_inFLASH(pDlm)) ;
	int xPos = 0 ;
	if (xMax == 0)
		xMax = strlen(pSrc);
	while (*pSrc && xMax) {
		int	xSrc = isupper((int) *pSrc) ? tolower((int) *pSrc) : (int) *pSrc ;
		const char * pTmp = pDlm ;
		while (*pTmp) {
			int	xDlm = isupper((int) *pTmp) ? tolower((int) *pTmp) : (int) *pTmp ;
			if (xSrc == xDlm)
				return xPos ;
			++pTmp ;
		}
		++xPos ;
		++pSrc ;
		--xMax ;
	}
	return erFAILURE ;
}

/**
 * @brief	Copies token from source buffer to destination buffer
 * @param	pDst - pointer to destination buffer
 * @param	pSrc - pointer to source buffer
 * @param	pDel - pointer to possible (leading) delimiters (to be skipped)
 * @param	flag - (< 0) force lower case, (= 0) no conversion (> 0) force upper case
 * @param	MaxLen - maximum number of characters in buffer
 * @return	pointer to next character to be processed...
 */
char * pcStringParseToken(char * pDst, char * pSrc, const char * pDel, int flag, size_t sDst) {
	IF_myASSERT(debugPARAM, halCONFIG_inSRAM(pDst) && halCONFIG_inMEM(pSrc) && halCONFIG_inMEM(pDel) && *pDel && sDst);
	IF_P(debugTRACK && ioB1GET(ioToken), "pSrc='%s'", pSrc);
	pSrc += xStringSkipDelim(pSrc, pDel, 0);			// skip over leading delims
	IF_P(debugTRACK && ioB1GET(ioToken), " -> '%s'", pSrc);
	char * pTmp = pDst;
	do {
		if ((*pSrc == 0) || strchr(pDel, *pSrc) != NULL) 			// end of string OR delimiter?
			break;
		*pTmp = (flag < 0) ? tolower((int)*pSrc) : (flag > 0) ? toupper((int)*pSrc) : *pSrc;
		++pTmp;
		++pSrc;
	} while (--sDst > 1);			// leave space for terminator
	*pTmp = 0;
	IF_P(debugTRACK && ioB1GET(ioToken), " -> '%s'  pDst='%s'\r\n", pSrc, pDst);
	return pSrc;					// pointer to NULL or next char [delimiter?] to be processed..
}

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
char * pcStringParseDateTime(char * pSrc, u64_t * pTStamp, struct tm * psTM) {
	IF_myASSERT(debugPARAM, halCONFIG_inMEM(pSrc) && halCONFIG_inSRAM(pTStamp) && halCONFIG_inSRAM(psTM)) ;
	u32_t flag = 0;
	/* TPmax	= ThisPar max length+1
	 * TPact	= ThisPar actual length ( <0=error  0=not found  >0=length )
	 * NPact	= NextPar actual length
	 * TPlim	= ThisPar max value */
	int	Value, TPlim, TPact, NPact;
	size_t TPmax;
	memset(psTM, 0, sizeof(struct tm));					// ensure all start as 0
	while (*pSrc == CHR_SPACE)
		++pSrc;											// make sure no leading spaces ....

	// check CCYY?MM? ahead
	TPact = xStringFindDelim(pSrc, delimDATE1, sizeof("CCYY")) ;
	NPact = (TPact > 0) ? xStringFindDelim(pSrc+TPact+1, delimDATE1, sizeof("MM")) : 0 ;
	IF_P(debugTRACK && ioB1GET(ioToken), "C: TPact=%d  NPact=%d", TPact, NPact) ;
	if (NPact >= 1) {
		IF_P(debugTRACK && ioB1GET(ioToken), "  Yr '%.*s'", TPact, pSrc) ;
		pSrc = pcStringParseValueRange(pSrc, (px_t) &Value, cvI32, NULL, (x32_t) 0, (x32_t) YEAR_BASE_MAX) ;
		IF_RETURN_X(pSrc == pcFAILURE, pSrc);
		// Cater for CCYY vs YY form
		psTM->tm_year = INRANGE(YEAR_BASE_MIN, Value, YEAR_BASE_MAX) ? Value - YEAR_BASE_MIN : Value ;
		flag |= DATETIME_YEAR_OK ;						// mark as done
		++pSrc ;										// skip over separator
	}
	IF_P(debugTRACK && ioB1GET(ioToken), "  Val=%d\r\n", psTM->tm_year) ;

	// check for MM?DD? ahead
	TPact = xStringFindDelim(pSrc, delimDATE1, sizeof("MM")) ;
	NPact = (TPact > 0) ? xStringFindDelim(pSrc+TPact+1, delimDATE2, sizeof("DD")) : 0 ;
	IF_P(debugTRACK && ioB1GET(ioToken), "M: TPact=%d  NPact=%d", TPact, NPact) ;

	if ((flag & DATETIME_YEAR_OK) || (NPact == 2) || (NPact == 0 && TPact > 0 && pSrc[TPact+3] == 0)) {
		IF_P(debugTRACK && ioB1GET(ioToken), "  Mon '%.*s'", TPact, pSrc) ;
		pSrc = pcStringParseValueRange(pSrc, (px_t) &Value, cvI32, NULL, (x32_t) 1, (x32_t) MONTHS_IN_YEAR) ;
		IF_RETURN_X(pSrc == pcFAILURE, pSrc);
		psTM->tm_mon = Value - 1 ;						// make 0 relative
		flag |= DATETIME_MON_OK ;						// mark as done
		++pSrc ;										// skip over separator
	}
	IF_P(debugTRACK && ioB1GET(ioToken), "  Val=%d\r\n", psTM->tm_mon) ;

	if (flag & (DATETIME_YEAR_OK | DATETIME_MON_OK)) {
		TPmax = sizeof("DD") ;
		TPlim = xTimeCalcDaysInMonth(psTM) ;
	} else {
		TPmax = sizeof("365") ;
		TPlim = DAYS_IN_YEAR ;
	}
	TPact = xStringFindDelim(pSrc, delimDATE2, TPmax) ;
	NPact = (TPact < 1 && pSrc[1] == 0) ? 1 : (TPact < 1 && pSrc[2] == 0) ? 2 : 0 ;
	IF_P(debugTRACK && ioB1GET(ioToken), "D: TPmax=%d  TPact=%d  NPact=%d  TPlim=%d", TPmax, TPact, NPact, TPlim) ;

	if ((flag & DATETIME_MON_OK) || (TPact > 0 && tolower((int) pSrc[TPact]) == CHR_t)) {
		IF_P(debugTRACK && ioB1GET(ioToken), "  Day '%.*s'", TPact, pSrc) ;
		pSrc = pcStringParseValueRange(pSrc, (px_t) &Value, cvI32, NULL, (x32_t) 1, (x32_t) TPlim) ;
		IF_RETURN_X(pSrc == pcFAILURE, pSrc);
		psTM->tm_mday = Value ;
		flag |= DATETIME_MDAY_OK ;						// mark as done
	}
	IF_P(debugTRACK && ioB1GET(ioToken), "  Val=%d\r\n", psTM->tm_mday) ;

	// calculate day of year ONLY if yyyy-mm-dd read in...
	if (flag == (DATETIME_YEAR_OK | DATETIME_MON_OK | DATETIME_MDAY_OK)) {
		psTM->tm_yday = xTimeCalcDaysYTD(psTM) ;
		flag |= DATETIME_YDAY_OK ;
	}

	// skip over 'T' if there
	if (*pSrc == CHR_T || *pSrc == CHR_t || *pSrc == CHR_SPACE)
		++pSrc;
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
	IF_P(debugTRACK && ioB1GET(ioToken), "H: TPmax=%d  TPact=%d  NPact=%d  TPlim=%d", TPmax, TPact, NPact, TPlim) ;

	if (NPact > 0) {
		IF_P(debugTRACK && ioB1GET(ioToken), "  Hr '%.*s'", TPact, pSrc) ;
		pSrc = pcStringParseValueRange(pSrc, (px_t) &Value, cvI32, NULL, (x32_t) 0, (x32_t) TPlim) ;
		IF_RETURN_X(pSrc == pcFAILURE, pSrc);
		psTM->tm_hour = Value ;
		flag	|= DATETIME_HOUR_OK ;					// mark as done
		++pSrc ;										// skip over separator
	}
	IF_P(debugTRACK && ioB1GET(ioToken), "  Val=%d\r\n", psTM->tm_hour) ;

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
	IF_P(debugTRACK && ioB1GET(ioToken), "M: TPmax=%d  TPact=%d  NPact=%d  TPlim=%d", TPmax, TPact, NPact, TPlim) ;

	if ((flag & DATETIME_HOUR_OK) || (NPact == 2) || (NPact == 0 && TPact > 0 && pSrc[TPact+3] == 0)) {
		IF_P(debugTRACK && ioB1GET(ioToken), "  Min '%.*s'", TPact, pSrc) ;
		pSrc = pcStringParseValueRange(pSrc, (px_t) &Value, cvI32, NULL, (x32_t) 0, (x32_t) TPlim) ;
		IF_RETURN_X(pSrc == pcFAILURE, pSrc);
		psTM->tm_min = Value ;
		flag	|= DATETIME_MIN_OK ;					// mark as done
		++pSrc ;										// skip over separator
	}
	IF_P(debugTRACK && ioB1GET(ioToken), "  Val=%d\r\n", psTM->tm_min) ;

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
	IF_P(debugTRACK && ioB1GET(ioToken), "S: TPmax=%d  TPact=%d  NPact=%d  TPlim=%d", TPmax, TPact, NPact, TPlim) ;

	if ((flag & DATETIME_MIN_OK) || (TPact > 0) || (INRANGE(1, NPact, --TPmax))) {
		IF_P(debugTRACK && ioB1GET(ioToken), "  Sec '%.*s'", TPact, pSrc) ;
		pSrc = pcStringParseValueRange(pSrc, (px_t) &Value, cvI32, NULL, (x32_t) 0, (x32_t) TPlim) ;
		IF_RETURN_X(pSrc == pcFAILURE, pSrc);
		psTM->tm_sec = Value ;
		flag	|= DATETIME_SEC_OK ;					// mark as done
	}
	IF_P(debugTRACK && ioB1GET(ioToken), "  Val=%d\r\n", psTM->tm_sec) ;

	// check for [.0{...}Z] and skip as appropriate
	int32_t uSecs = 0 ;
	TPact = xStringFindDelim(pSrc, ".s", 1) ;
	TPmax = sizeof("999999") ;
	if (TPact == 0) {
		++pSrc ;										// skip over '.'
		TPact = xStringFindDelim(pSrc, "z ", TPmax) ;	// find position of Z/z in buffer
		if (OUTSIDE(1, TPact, TPmax)) {
		/* XXX valid terminator not found, but maybe a NUL ?
		 * still a problem, what about junk after the last number ? */
			NPact = strlen(pSrc);
			if (OUTSIDE(1, NPact, --TPmax))
				return pcFAILURE;
			TPact = NPact ;
		}
		IF_P(debugTRACK && ioB1GET(ioToken), " uS '%.*s'", TPact, pSrc) ;
		pSrc = pcStringParseValueRange(pSrc, (px_t) &uSecs, cvI32, NULL, (x32_t) 0, (x32_t) (MICROS_IN_SECOND-1)) ;
		IF_RETURN_X(pSrc == pcFAILURE, pSrc);
		TPact = 6 - TPact ;
		while (TPact--) uSecs *= 10;
		flag |= DATETIME_MSEC_OK ;						// mark as done
		IF_P(debugTRACK && ioB1GET(ioToken), "  Val=%d\r\n", uSecs) ;
	}

	if (pSrc[0] == CHR_Z || pSrc[0] == CHR_z)
		++pSrc;											// skip over trailing 'Z'

	u32_t Secs ;
	if (flag & DATETIME_YEAR_OK) {						// full timestamp data found?
		psTM->tm_wday = (xTimeCalcDaysToDate(psTM) + timeEPOCH_DAY_0_NUM) % DAYS_IN_WEEK ;
		psTM->tm_yday = xTimeCalcDaysYTD(psTM) ;
		Secs = xTimeCalcSeconds(psTM, 0) ;
	} else {
		Secs = xTimeCalcSeconds(psTM, 1) ;
	}
	*pTStamp = xTimeMakeTimestamp(Secs, uSecs);
	IF_P(debugTRACK && ioB1GET(ioToken), "flag=%p  uS=%`llu  wday=%d  yday=%d  y=%d  m=%d  d=%d  %dh%02dm%02ds\r\n",
			flag, *pTStamp, psTM->tm_wday, psTM->tm_yday, psTM->tm_year, psTM->tm_mon,
			psTM->tm_mday, psTM->tm_hour, psTM->tm_min, psTM->tm_sec);
	return pSrc;
}

// ############################## Bitmap to string decode functions ################################

int	xBitMapDecodeChanges(u32_t Val1, u32_t Val2, u32_t Mask, const char * const pMesArray[], int Flag, char * pcBuf, size_t BufSize) {
	int	pos, idx, BufLen = 0;
	u32_t CurMask, C1, C2;
	const char * pFormat = (Flag & bmdcCOLOUR) ? " %C%s%C" : " %c%s%c";
	for (pos = 31, idx = 31, CurMask = 0x80000000 ; pos >= 0; CurMask >>= 1, --pos, --idx) {
		if (Mask & CurMask) {
			if ((Val1 & CurMask) && (Val2 & CurMask)) {	// No change, was 1 still 1
				C1 = (Flag & bmdcCOLOUR) ? colourFG_WHITE : CHR_TILDE;
				C2 = (Flag & bmdcCOLOUR) ? attrRESET : CHR_TILDE;
			} else if (Val1 & CurMask) {				// 1 -> 0
				C1 = (Flag & bmdcCOLOUR) ? colourFG_RED : CHR_UNDERSCORE;
				C2 = (Flag & bmdcCOLOUR) ? attrRESET : CHR_UNDERSCORE;
			} else if (Val2 & CurMask) {				// 0 -> 1
				C1 = (Flag & bmdcCOLOUR) ? colourFG_GREEN : CHR_CARET ;
				C2 = (Flag & bmdcCOLOUR) ? attrRESET : CHR_CARET;
			} else {									// No change, was 0 still 0
				C1 = 0;
			}
			if (C1)	{							// only show if true or changed
				BufLen += snprintfx(pcBuf+BufLen, BufSize-BufLen, pFormat,
					C1, pMesArray[idx], C2);
			}
		}
	}
	if (Flag & bmdcNEWLINE)
		BufLen += snprintfx(pcBuf+BufLen, BufSize-BufLen, strCRLF);

	return BufLen ;
}

char * pcBitMapDecodeChanges(u32_t Val1, u32_t Val2, u32_t Mask, const char * const pMesArray[], int Flag) {
	char * pcBuf = pvRtosMalloc(controlSIZE_FLAGS_BUF) ;
	xBitMapDecodeChanges(Val1, Val2, Mask, pMesArray, Flag, pcBuf, controlSIZE_FLAGS_BUF) ;
	return pcBuf ;
}

int	xBitMapDecode(u32_t Value, u32_t Mask, const char * const pMesArray[], char * pBuf, size_t BufSize) {
	int	pos, idx, BufLen = 0;
	u32_t CurMask;
	u8_t Pad = CHR_NUL;
	for (pos = 31, idx = 31, CurMask = 0x80000000 ; pos >= 0; CurMask >>= 1, --pos, --idx) {
		if ((Mask & CurMask) && (Value & CurMask)) {
			BufLen += snprintfx(pBuf + BufLen, BufSize - BufLen, "%c%s", Pad, pMesArray[idx]);
			Pad = CHR_SPACE;
		}
	}
	return BufLen ;
}

char * pcBitMapDecode(u32_t Value, u32_t Mask, const char * const pMesArray[]) {
	char * pcBuf = pvRtosMalloc(controlSIZE_FLAGS_BUF) ;
	xBitMapDecode(Value, Mask, pMesArray, pcBuf, controlSIZE_FLAGS_BUF) ;
	return pcBuf ;
}

/**
 * vBitMapDecode() - decodes a bitmapped value using a bit-mapped mask
 * @brief
 * @param	Value - 32bit value to the decoded
 * @param	Mask - 32bit mask to be applied
 * @param	pMesArray - pointer to array of messages (1 per bit SET in the mask)
 * return	none
 */
void vBitMapDecode(u32_t Value, u32_t Mask, const char * const pMesArray[]) {
	int	pos, idx ;
	u32_t	CurMask ;
	if (Mask) {
		for (pos = 31, idx = 0, CurMask = 0x80000000 ; pos >= 0; CurMask >>= 1, --pos) {
			if (CurMask & Mask & Value)
				P(" |%02d|%s|", pos, pMesArray[idx]) ;
			if (CurMask & Mask)
				idx++ ;
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
void vBitMapReport(char * pName, u32_t Value, u32_t Mask, const char * pMesArray[]) {
	IF_myASSERT(debugPARAM, halCONFIG_inMEM(pMesArray)) ;
	if (pName != NULL)
		P(" %s 0x%02x:", pName, Value) ;
	vBitMapDecode(Value, Mask, pMesArray) ;
	if (pName != NULL)
		P(strCRLF) ;
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
int	xStringValueMap(const char * pString, char * pBuf, u32_t uValue, int iWidth) {
	IF_myASSERT(debugPARAM, halCONFIG_inFLASH(pString) && halCONFIG_inSRAM(pBuf) && (iWidth <= 32) && (strnlen(pString, 33) <= iWidth)) ;
	u32_t uMask = 0x8000 >> (32 - iWidth) ;
	int Idx ;
	for (Idx = 0; Idx < iWidth; ++Idx, ++pString, ++pBuf, uMask >>= 1)
		*pBuf = (uValue | uMask) ? *pString : CHR_MINUS;
	*pBuf = 0 ;
	return Idx ;
}

// #################################################################################################

#define	stringTEST_FLAG			0x0000

#define	stringTEST_EPOCH		(stringTEST_FLAG & 0x0001)
#define	stringTEST_DATES		(stringTEST_FLAG & 0x0002)
#define	stringTEST_TIMES		(stringTEST_FLAG & 0x0004)
#define	stringTEST_DTIME		(stringTEST_FLAG & 0x0008)

#define	stringTEST_RELDAT		(stringTEST_FLAG & 0x0010)
#define	stringTEST_PARSE		(stringTEST_FLAG & 0x0020)

void x_string_general_test(void) {
#if	(stringTEST_FLAG & (stringTEST_EPOCH|stringTEST_DATES|stringTEST_TIMES|stringTEST_DTIME|stringTEST_RELDAT))
	struct	tm	sTM;
	tsz_t	sTSZ;
#endif

#if (stringTEST_PARSE)
	char caSrc[] = ";,Twenty*Two*Character_s ,;Twenty_Three_Characters, ;_Twenty_Four_Characters_, ; _Twenty_Five_Character[s] ;,";
	char caBuf[24];
	ioB1SET(ioToken,1);
	char * pTmp = pcStringParseToken(caBuf, caSrc, " ,;", 0, sizeof(caBuf));
	pTmp = pcStringParseToken(caBuf, pTmp, " ,;", 0, sizeof(caBuf));
	pTmp = pcStringParseToken(caBuf, pTmp, " ,;", 0, sizeof(caBuf));
	pTmp = pcStringParseToken(caBuf, pTmp, " ,;", 0, sizeof(caBuf));
	pTmp = pcStringParseToken(caBuf, pTmp, " ,;", 0, sizeof(caBuf));
#endif

#if	(stringTEST_EPOCH)
	chartest[64] ;
	sTSZ.usecs	= xTimeMakeTimestamp(SECONDS_IN_EPOCH_PAST, 0) ;
	xsnprintf(test, sizeof(test), "%.6Z", &sTSZ) ;
	pcStringParseDateTime(test, &sTSZ.usecs, &sTM) ;
	printfx("Input: %s => Date: %s %02d %s %04d Time: %02d:%02d:%02d Day# %d\r\n",
			test, xTime_GetDayName(sTM.tm_wday), sTM.tm_mday, xTime_GetMonthName(sTM.tm_mon), sTM.tm_year + YEAR_BASE_MIN,
			sTM.tm_hour, sTM.tm_min, sTM.tm_sec, sTM.tm_yday) ;

	sTSZ.usecs	= xTimeMakeTimestamp(SECONDS_IN_EPOCH_FUTURE, 999999) ;
	xsnprintf(test, sizeof(test), "%.6Z", &sTSZ) ;
	pcStringParseDateTime(test, &sTSZ.usecs, &sTM) ;
	printfx("Input: %s => Date: %s %02d %s %04d Time: %02d:%02d:%02d Day# %d\r\n",
			test, xTime_GetDayName(sTM.tm_wday), sTM.tm_mday, xTime_GetMonthName(sTM.tm_mon), sTM.tm_year + YEAR_BASE_MIN,
			sTM.tm_hour, sTM.tm_min, sTM.tm_sec, sTM.tm_yday) ;
#endif

#if	(stringTEST_DATES)
	pcStringParseDateTime((char *) "2019/04/15", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "2019/04/15 ", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "2019/04/15T", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "2019/04/15t", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;

	pcStringParseDateTime((char *) "2019-04-15", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "2019-04-15 ", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "2019-04-15T", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "2019-04-15t", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
#endif

#if	(stringTEST_TIMES)
	pcStringParseDateTime((char *) "1", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "1 ", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "01", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "01Z", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "1z", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;

	pcStringParseDateTime((char *) "1.1", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1100000 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "1.1 ", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1100000 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "1.1Z", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1100000 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "1.1z", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1100000 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;

	pcStringParseDateTime((char *) "1.000001", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1000001 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "1.000001 ", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1000001 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "1.000001Z", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1000001 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "1.000001z", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1000001 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
#endif

#if	(stringTEST_DTIME)
	pcStringParseDateTime((char *) "2019/04/15T1:23:45.678901Z", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=1 || sTM.tm_min!=23 || sTM.tm_sec!=45 || (sTSZ.usecs % MILLION) != 678901 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;

	pcStringParseDateTime((char *) "2019-04-15t01h23m45s678901z", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=1 || sTM.tm_min!=23 || sTM.tm_sec!=45 || (sTSZ.usecs % MILLION) != 678901 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;

	pcStringParseDateTime((char *) "2019/04-15 1:23m45.678901", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=1 || sTM.tm_min!=23 || sTM.tm_sec!=45 || (sTSZ.usecs % MILLION) != 678901 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;

	pcStringParseDateTime((char *) "2019-04/15t01h23:45s678901", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=1 || sTM.tm_min!=23 || sTM.tm_sec!=45 || (sTSZ.usecs % MILLION) != 678901 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
#endif

#if	(stringTEST_RELDAT)
	pcStringParseDateTime((char *) "1/1-1", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=1 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
	pcStringParseDateTime((char *) "1-1/1", &sTSZ.usecs, &sTM) ;
	printfx(sTM.tm_year!=1 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed\r\n" : " #%d Passed\r\n", __LINE__) ;
#endif
}
