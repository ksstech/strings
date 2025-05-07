// string_general.c - Copyright (c) 2014-25 Andre M. Maree / KSS Technologies (Pty) Ltd.

#include "hal_platform.h"
#include "hal_memory.h"
#include "printfx.h"
#include "hal_options.h"
#include "errors_events.h"
#include "string_general.h"
#include "string_to_values.h"

#include <string.h>
#include <ctype.h>

// ########################################### Macros ##############################################

#define	debugFLAG					0xF000
#define	debugPARSE_ENCODED			(debugFLAG & 0x0100)
#define	debugDELIM					(debugFLAG & 0x0200)
#define	debugTIMING					(debugFLAG_GLOBAL & debugFLAG & 0x1000)
#define	debugTRACK					(debugFLAG_GLOBAL & debugFLAG & 0x2000)
#define	debugPARAM					(debugFLAG_GLOBAL & debugFLAG & 0x4000)
#define	debugRESULT					(debugFLAG_GLOBAL & debugFLAG & 0x8000)

#ifndef stringMAX_LEN
	#define	stringMAX_LEN			2048
#endif

#define	delimDATE1	"-/"
#define	delimDATE2	"t "
#define	delimTIME1	"h:"
#define	delimTIME2	"m:"
#define	delimTIME3	"sz. "

#ifdef appOPTIONS
	#define OPT_GET(opt)	xOptionGet(opt)
#else
	#define OPT_GET(opt)	0		
#endif

// ###################################### Public functions #########################################

int	xstrverify(char * pStr, char cMin, char cMax, char cNum) {
	if (*pStr == 0)
		return erFAILURE;
	while (cNum--) {
		if (OUTSIDE(cMin, *pStr, cMax))
			return erFAILURE;
	}
	return erSUCCESS;
}

size_t xstrnlen(const char * pStr, size_t uMax) {
	IF_myASSERT(debugPARAM, halMemoryANY((void *)pStr));
	IF_myASSERT(debugPARAM, uMax > 0);
	size_t uNow;
	for (uNow = 0; (*pStr != 0) && (uNow < uMax); ++pStr, ++uNow);
	return uNow;
}

int	xstrncpy(char * pDst, char * pSrc, int xLen ) {
	IF_myASSERT(debugPARAM, halMemorySRAM((void*) pDst) && halMemoryANY((void*) pSrc) && xLen);
	int Cnt = 0;
	while (*pSrc != 0 && Cnt < xLen) {
		*pDst++ = *pSrc++;								// copy across and adjust both pointers
		Cnt++;											// adjust length copied
	}
	if (Cnt < xLen)
		*pDst = 0;										// if space left, terminate
	return Cnt;
}

int	xmemrev(char * pMem, size_t Size) {
	IF_myASSERT(debugPARAM, halMemorySRAM((void*) pMem) && Size > 1);
	if (pMem == NULL || Size < 2) return erFAILURE;
	char * pRev = pMem + Size - 1;
#if (stringXMEMREV_XOR == 1)
	for (char * pFwd = pMem; pRev > pFwd; ++pFwd, --pRev) {
		*pFwd ^= *pRev;
		*pRev ^= *pFwd;
		*pFwd ^= *pRev;
	}
#else
	while (pMem < pRev) {
		char cTemp = *pMem;
		*pMem++	= *pRev;
		*pRev--	= cTemp;
	}
#endif
	return erSUCCESS;
}

void xstrrev(char * pStr) { xmemrev(pStr, strlen(pStr)); }

int	strchr_i(const char * pStr, int iChr) {
	IF_myASSERT(debugPARAM, halMemoryANY((void *)pStr));
	char * pTmp = strchr(pStr, iChr);
	return pTmp ? pTmp - pStr : erFAILURE;
}

int	xstrncmp(const char * s1, const char * s2, size_t xL, bool Exact) {
	if (xL == 0) {
		size_t sz1 = strlen(s1);
		size_t sz2 = strlen(s2);
		if (sz1 != sz2)
			return 0;
		xL = sz1;
	}
	IF_myASSERT(debugPARAM, halMemoryANY((void *)s1) && halMemoryANY((void *)s2));
	while (*s1 && *s2 && xL) {
		if (Exact == true) {
			if (*s1 != *s2)
				break;
		} else {
			if (toupper((int)*s1) != toupper((int)*s2))
				break;
		}
		++s1;
		++s2;
		--xL;
	}
	return (*s1 == 0 && *s2 == 0) ? 1 : (xL == 0) ? 1 : 0;
}

int	xstrcmp(const char * s1, const char * s2, bool Exact) {
	IF_myASSERT(debugPARAM, halMemoryANY((void *)s1) && halMemoryANY((void *)s2));
	while (*s1 && *s2) {
		if (Exact) {
			if (*s1 != *s2)
				break;
		} else {
			if (toupper((int)*s1) != toupper((int)*s2))
				break;
		}
		++s1;
		++s2;
	}
	return (*s1 == 0 && *s2 == 0) ? 1 : 0;
}

int	xstrindex(char * key, char * array[]) {
	int	i = 0;
	while (array[i]) {
		if (strcasecmp(key, array[i]) == 0)
			return i;									// strings match, return index
		++i;
	}
	return erFAILURE;
}

int xstrishex(char * pStr) {
	int iRV = 0;
	while (*pStr == CHR_SPACE) {						// leading ' '
		++pStr;
		++iRV;
	}
	if (*pStr == CHR_0) {								// leading '0' ?
		++pStr;
		++iRV;
	}
	if (*pStr == CHR_X || *pStr == CHR_x)
		return ++iRV;									// leading 'X' or 'x' ?
	return 0;
}

// ############################### string parsing routines #########################################

int	xStringParseEncoded(char * pDst, char * pSrc) {
	IF_myASSERT(debugPARAM, halMemorySRAM((void*) pSrc));
	IF_myASSERT(debugPARAM && (pDst != NULL), halMemorySRAM((void*) pDst));
	int iRV = 0;
	if (pDst == NULL)
		pDst = pSrc;
	IF_PX(debugPARSE_ENCODED, "%s  ", pSrc);
	while(*pSrc != 0) {
		if (*pSrc == CHR_PERCENT) {						// escape char?
			int Val1 = xHexCharToValue(*++pSrc, BASE16);	// yes, parse 1st value
			if (Val1 == erFAILURE)
				return erFAILURE;
			int Val2 = xHexCharToValue(*++pSrc, BASE16);	// parse 2nd value
			if (Val2 == erFAILURE)
				return erFAILURE;
			IF_PX(debugPARSE_ENCODED, "[%d+%d=%d]  ", Val1, Val2, (Val1 << 4) + Val2);
			*pDst++ = (Val1 << 4) + Val2;				// calc & store final value
			++pSrc;									// step to next char
		} else {
			*pDst++ = *pSrc++;							// copy as is to (new) position
		}
		++iRV;											// & adjust count...
	}
	*pDst = 0;
	IF_PX(debugPARSE_ENCODED, "%s" strNL, pSrc-iRV);
	return iRV;
}

int	xStringParseUnicode(char * pDst, char * pSrc, size_t Len) {
	IF_myASSERT(debugPARAM, halMemorySRAM((void*) pSrc));
	IF_myASSERT(debugPARAM && (pDst != NULL), halMemorySRAM((void*) pDst));
	int iRV = 0;
	if (pDst == NULL)
		pDst = pSrc;
	IF_PX(debugPARSE_ENCODED, "%s  ", pSrc);
	while(*pSrc != 0 && (iRV < Len)) {
		if (*pSrc == CHR_BACKSLASH && *(pSrc+1) == CHR_u) {		// escape chars?
			int Val = 0;
			for (int i = 2; i < 6; ++i) {
				int iRV2 = xHexCharToValue(pSrc[i], BASE16);
				IF_PX(debugPARSE_ENCODED, "%c/%d/%d  ",pSrc[i], iRV2, Val);
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
			IF_PX(debugPARSE_ENCODED, "%d  ", Val);
		} else {
			*pDst++ = *pSrc++;							// copy as is to (new) position
		}
		++iRV;											// & adjust count...
	}
	if (iRV < Len)
		*pDst = 0;
	IF_PX(debugPARSE_ENCODED, "%.*s" strNL, iRV, pDst-iRV);
	return iRV;
}

int	xStringSkipDelim(char * pSrc, const char * pDel, size_t MaxLen) {
	IF_myASSERT(debugPARAM, halMemoryANY((void*) pSrc) && halMemoryANY((void *)pDel));
	// If no length supplied
	if (MaxLen == 0) {
		MaxLen = xstrnlen(pSrc, stringMAX_LEN);	// assume NULL terminated and calculate length
		IF_myASSERT(debugRESULT, MaxLen < stringMAX_LEN);		// just a check to verify not understated
	}
	IF_PX(debugDELIM, " '%.4s'", pSrc);
	// continue skipping over valid terminator characters
	int	CurLen = 0;
	while (strchr(pDel, *pSrc) && (CurLen < MaxLen)) {
		++pSrc;
		++CurLen;
	}
	IF_PX(debugDELIM, "->'%.4s'", pSrc);
	return CurLen;								// number of delimiters skipped over
}

int xStringCountSpaces(char * pSrc) {
	int iRV = 0;
	while (isblank(*pSrc)) {
		++pSrc;
		++iRV;
	}
	return iRV;
}

int xStringCountCRLF(char * pSrc) {
	int iRV = 0;
	while (*pSrc == CHR_CR || *pSrc == CHR_LF) {
		++pSrc;
		++iRV;
	}
	return iRV;
}

int	xStringFindDelim(char * pSrc, const char * pDlm, size_t xMax) {
	IF_myASSERT(debugPARAM, halMemoryANY(pSrc));
	int xPos = 0;
	if (xMax == 0)
		xMax = strlen(pSrc);
	while (*pSrc && xMax) {
		int	xSrc = isupper((int) *pSrc) ? tolower((int) *pSrc) : (int) *pSrc;
		const char * pTmp = pDlm;
		while (*pTmp) {
			int	xDlm = isupper((int) *pTmp) ? tolower((int) *pTmp) : (int) *pTmp;
			if (xSrc == xDlm)
				return xPos;
			++pTmp;
		}
		++xPos;
		++pSrc;
		--xMax;
	}
	return erFAILURE;
}

char * pcStringParseToken(char * pDst, char * pSrc, const char * pDel, int flag, size_t sDst) {
	pSrc += xStringCountSpaces(pSrc);					// skip over leading "spaces"
	char * pTmp = pDst;
	do {
		if (*pSrc == 0 || strchr(pDel, *pSrc))			// end of string OR delimiter?
			break;
		*pTmp = (flag < 0) ? tolower((int)*pSrc) : (flag > 0) ? toupper((int)*pSrc) : *pSrc;
		++pTmp;
		++pSrc;
	} while (--sDst > 1);			// leave space for terminator
	*pTmp = 0;
	return pSrc;					// pointer to NULL or next char [delimiter?] to be processed..
}

char * pcStringParseDateTime(char * pSrc, u64_t * pTStamp, struct tm * psTM) {
	u32_t flag = 0;
	/* TPmax	= ThisPar max length+1
	 * TPact	= ThisPar actual length ( <0=error  0=not found  >0=length )
	 * NPact	= NextPar actual length
	 * TPlim	= ThisPar max value */
	int	Value, TPlim, TPact, NPact;
	size_t TPmax;
	memset(psTM, 0, sizeof(struct tm));					// ensure all start as 0
	while (*pSrc == CHR_SPACE)							// make sure no leading spaces ....
		++pSrc;

	// check CCYY?MM? ahead
	TPact = xStringFindDelim(pSrc, delimDATE1, sizeof("CCYY"));
	NPact = (TPact > 0) ? xStringFindDelim(pSrc+TPact+1, delimDATE1, sizeof("MM")) : 0;
	u8_t Option = OPT_GET(dbgSyntax);
	IF_PX(debugTRACK && Option, "C: TPact=%d  NPact=%d", TPact, NPact);
	if (NPact >= 1) {
		IF_PX(debugTRACK && Option, "  Yr '%.*s'", TPact, pSrc);
		pSrc = cvParseRangeX32(pSrc, (px_t) &Value, cvI32, (x32_t) 0, (x32_t) YEAR_BASE_MAX);
		IF_RETURN_X(pSrc == pcFAILURE, pSrc);
		// Cater for CCYY vs YY form
		psTM->tm_year = INRANGE(YEAR_BASE_MIN, Value, YEAR_BASE_MAX) ? Value - YEAR_BASE_MIN : Value;
		flag |= DATETIME_YEAR_OK;						// mark as done
		++pSrc;										// skip over separator
	}
	IF_PX(debugTRACK && Option, "  Val=%d" strNL, psTM->tm_year);

	// check for MM?DD? ahead
	TPact = xStringFindDelim(pSrc, delimDATE1, sizeof("MM"));
	NPact = (TPact > 0) ? xStringFindDelim(pSrc+TPact+1, delimDATE2, sizeof("DD")) : 0;
	IF_PX(debugTRACK && Option, "M: TPact=%d  NPact=%d", TPact, NPact);

	if ((flag & DATETIME_YEAR_OK) || (NPact == 2) || (NPact == 0 && TPact > 0 && pSrc[TPact+3] == 0)) {
		IF_PX(debugTRACK && Option, "  Mon '%.*s'", TPact, pSrc);
		pSrc = cvParseRangeX32(pSrc, (px_t) &Value, cvI32, (x32_t) 1, (x32_t) MONTHS_IN_YEAR);
		IF_RETURN_X(pSrc == pcFAILURE, pSrc);
		psTM->tm_mon = Value - 1;						// make 0 relative
		flag |= DATETIME_MON_OK;						// mark as done
		++pSrc;										// skip over separator
	}
	IF_PX(debugTRACK && Option, "  Val=%d" strNL, psTM->tm_mon);

	if (flag & (DATETIME_YEAR_OK | DATETIME_MON_OK)) {
		TPmax = sizeof("DD");
		TPlim = xTimeCalcDaysInMonth(psTM);
	} else {
		TPmax = sizeof("365");
		TPlim = DAYS_IN_YEAR;
	}
	TPact = xStringFindDelim(pSrc, delimDATE2, TPmax);
	NPact = (TPact < 1 && pSrc[1] == 0) ? 1 : (TPact < 1 && pSrc[2] == 0) ? 2 : 0;
	IF_PX(debugTRACK && Option, "D: TPmax=%d  TPact=%d  NPact=%d  TPlim=%d", TPmax, TPact, NPact, TPlim);

	if ((flag & DATETIME_MON_OK) || (TPact > 0 && tolower((int) pSrc[TPact]) == CHR_t)) {
		IF_PX(debugTRACK && Option, "  Day '%.*s'", TPact, pSrc);
		pSrc = cvParseRangeX32(pSrc, (px_t) &Value, cvI32, (x32_t) 1, (x32_t) TPlim);
		IF_RETURN_X(pSrc == pcFAILURE, pSrc);
		psTM->tm_mday = Value;
		flag |= DATETIME_MDAY_OK;						// mark as done
	}
	IF_PX(debugTRACK && Option, "  Val=%d" strNL, psTM->tm_mday);

	// calculate day of year ONLY if yyyy-mm-dd read in...
	if (flag == (DATETIME_YEAR_OK | DATETIME_MON_OK | DATETIME_MDAY_OK)) {
		psTM->tm_yday = xTimeCalcDaysYTD(psTM);
		flag |= DATETIME_YDAY_OK;
	}

	// skip over 'T' if there
	if (*pSrc == CHR_T || *pSrc == CHR_t || *pSrc == CHR_SPACE)
		++pSrc;
	// check for HH?MM?
	if (flag & (DATETIME_YEAR_OK | DATETIME_MON_OK | DATETIME_MDAY_OK)) {
		TPmax = sizeof("HH");
		TPlim = HOURS_IN_DAY - 1;
	} else {
		TPmax = sizeof("8760");
		TPlim = HOURS_IN_YEAR;
	}
	TPact = xStringFindDelim(pSrc, delimTIME1, TPmax);
	NPact = (TPact > 0) ? xStringFindDelim(pSrc+TPact+1, delimTIME2, sizeof("HH")) : 0;
	IF_PX(debugTRACK && Option, "H: TPmax=%d  TPact=%d  NPact=%d  TPlim=%d", TPmax, TPact, NPact, TPlim);

	if (NPact > 0) {
		IF_PX(debugTRACK && Option, "  Hr '%.*s'", TPact, pSrc);
		pSrc = cvParseRangeX32(pSrc, (px_t) &Value, cvI32, (x32_t) 0, (x32_t) TPlim);
		IF_RETURN_X(pSrc == pcFAILURE, pSrc);
		psTM->tm_hour = Value;
		flag	|= DATETIME_HOUR_OK;					// mark as done
		++pSrc;										// skip over separator
	}
	IF_PX(debugTRACK && Option, "  Val=%d" strNL, psTM->tm_hour);

	// check for MM?SS?
	// [M..]M{m:}[S]S{s.Zz }
	if (flag & (DATETIME_YEAR_OK | DATETIME_MON_OK | DATETIME_MDAY_OK | DATETIME_HOUR_OK)) {
		TPmax = sizeof("MM");
		TPlim = MINUTES_IN_HOUR-1;
	} else {
		TPmax = sizeof("525600");
		TPlim = MINUTES_IN_YEAR;
	}
	TPact = xStringFindDelim(pSrc, delimTIME2, TPmax);
	NPact = (TPact > 0) ? xStringFindDelim(pSrc+TPact+1, delimTIME3, sizeof("SS")) : 0;
	IF_PX(debugTRACK && Option, "M: TPmax=%d  TPact=%d  NPact=%d  TPlim=%d", TPmax, TPact, NPact, TPlim);

	if ((flag & DATETIME_HOUR_OK) || (NPact == 2) || (NPact == 0 && TPact > 0 && pSrc[TPact+3] == 0)) {
		IF_PX(debugTRACK && Option, "  Min '%.*s'", TPact, pSrc);
		pSrc = cvParseRangeX32(pSrc, (px_t) &Value, cvI32, (x32_t) 0, (x32_t) TPlim);
		IF_RETURN_X(pSrc == pcFAILURE, pSrc);
		psTM->tm_min = Value;
		flag	|= DATETIME_MIN_OK;					// mark as done
		++pSrc;										// skip over separator
	}
	IF_PX(debugTRACK && Option, "  Val=%d" strNL, psTM->tm_min);

	/*
	 * To support parsing of long (>60s) RELATIVE time period we must support values
	 * much bigger than 59.
	 * If NONE of the CCYY?MM?DD?HH?MM portions supplied then must be
	 *		SS[s.Zz ]
	 * else we must allow for
	 * 		SSSSSSSS[s.Zz ]
	 */
	if (flag & (DATETIME_YEAR_OK | DATETIME_MON_OK | DATETIME_MDAY_OK | DATETIME_HOUR_OK | DATETIME_MIN_OK)) {
		TPmax = sizeof("SS");
		TPlim = SECONDS_IN_MINUTE - 1;
	} else {
		TPmax = sizeof("31622399");
		TPlim = SECONDS_IN_LEAPYEAR - 1;
	}
	TPact = xStringFindDelim(pSrc, delimTIME3, TPmax);
	NPact = (TPact < 1) ? strlen(pSrc) : 0;
	IF_PX(debugTRACK && Option, "S: TPmax=%d  TPact=%d  NPact=%d  TPlim=%d", TPmax, TPact, NPact, TPlim);

	if ((flag & DATETIME_MIN_OK) || (TPact > 0) || (INRANGE(1, NPact, --TPmax))) {
		IF_PX(debugTRACK && Option, "  Sec '%.*s'", TPact, pSrc);
		pSrc = cvParseRangeX32(pSrc, (px_t) &Value, cvI32, (x32_t) 0, (x32_t) TPlim);
		IF_RETURN_X(pSrc == pcFAILURE, pSrc);
		psTM->tm_sec = Value;
		flag	|= DATETIME_SEC_OK;					// mark as done
	}
	IF_PX(debugTRACK && Option, "  Val=%d" strNL, psTM->tm_sec);

	// check for [.0{...}Z] and skip as appropriate
	int32_t uSecs = 0;
	TPact = xStringFindDelim(pSrc, ".s", 1);
	TPmax = sizeof("999999");
	if (TPact == 0) {
		++pSrc;										// skip over '.'
		TPact = xStringFindDelim(pSrc, "z ", TPmax);	// find position of Z/z in buffer
		if (OUTSIDE(1, TPact, TPmax)) {
		/* XXX valid terminator not found, but maybe a NUL ?
		 * still a problem, what about junk after the last number ? */
			NPact = strlen(pSrc);
			if (OUTSIDE(1, NPact, --TPmax))
				return pcFAILURE;
			TPact = NPact;
		}
		IF_PX(debugTRACK && Option, " uS '%.*s'", TPact, pSrc);
		pSrc = cvParseRangeX32(pSrc, (px_t) &uSecs, cvI32, (x32_t) 0, (x32_t) (MICROS_IN_SECOND-1));
		IF_RETURN_X(pSrc == pcFAILURE, pSrc);
		TPact = 6 - TPact;
		while (TPact--) uSecs *= 10;
		flag |= DATETIME_MSEC_OK;						// mark as done
		IF_PX(debugTRACK && Option, "  Val=%ld" strNL, uSecs);
	}

	if (pSrc[0] == CHR_Z || pSrc[0] == CHR_z)
		++pSrc;											// skip over trailing 'Z'

	u32_t Secs;
	if (flag & DATETIME_YEAR_OK) {						// full timestamp data found?
		psTM->tm_wday = (xTimeCalcDaysToDate(psTM) + timeEPOCH_DAY_0_NUM) % DAYS_IN_WEEK;
		psTM->tm_yday = xTimeCalcDaysYTD(psTM);
		Secs = xTimeCalcSeconds(psTM, 0);
	} else {
		Secs = xTimeCalcSeconds(psTM, 1);
	}
	*pTStamp = xTimeMakeTimeStamp(Secs, uSecs);
	IF_PX(debugTRACK && Option, "flag=%p  uS=%'llu  wday=%d  yday=%d  y=%d  m=%d  d=%d  %dh%02dm%02ds" strNL,
			(void *) flag, *pTStamp, psTM->tm_wday, psTM->tm_yday, psTM->tm_year,
			psTM->tm_mon, psTM->tm_mday, psTM->tm_hour, psTM->tm_min, psTM->tm_sec);
	return pSrc;
}

// ############################## Bitmap to string decode functions ################################

int	xStringValueMap(const char * pString, char * pBuf, u32_t uValue, int iWidth) {
	IF_myASSERT(debugPARAM, halMemoryANY((void*) pString) && halMemorySRAM((void*) pBuf) && (iWidth <= 32) && (strnlen(pString, 33) <= iWidth));
	u32_t uMask = 0x8000 >> (32 - iWidth);
	int Idx;
	for (Idx = 0; Idx < iWidth; ++Idx, ++pString, ++pBuf, uMask >>= 1)
		*pBuf = (uValue | uMask) ? *pString : CHR_MINUS;
	*pBuf = 0;
	return Idx;
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
	#if	(stringTEST_EPOCH || stringTEST_DATES || stringTEST_TIMES || stringTEST_DTIME || stringTEST_RELDAT)
	struct tm sTM;
	tsz_t sTSZ;
	#endif

	#if (stringTEST_PARSE)
	char caSrc[] = ";,Twenty*Two*Character_s ,;Twenty_Three_Characters, ;_Twenty_Four_Characters_, ; _Twenty_Five_Character[s] ;,";
	char caBuf[24];
	#if (appOPTIONS == 1)
		xOptionSet(dbgSyntax,1);
	#else
		#warning "Options support required for proper functioning!!!"
	#endif
	char * pTmp = pcStringParseToken(caBuf, caSrc, " ,;", 0, sizeof(caBuf));
	pTmp = pcStringParseToken(caBuf, pTmp, " ,;", 0, sizeof(caBuf));
	pTmp = pcStringParseToken(caBuf, pTmp, " ,;", 0, sizeof(caBuf));
	pTmp = pcStringParseToken(caBuf, pTmp, " ,;", 0, sizeof(caBuf));
	pTmp = pcStringParseToken(caBuf, pTmp, " ,;", 0, sizeof(caBuf));
	#endif

	#if	(stringTEST_EPOCH)
	char test[64];
	sTSZ.usecs	= xTimeMakeTimeStamp(SECONDS_IN_EPOCH_PAST, 0);
	snprintfx(test, sizeof(test), "%.6Z", &sTSZ);
	pcStringParseDateTime(test, &sTSZ.usecs, &sTM);
	PX("Input: %s => Date: %s %02d %s %04d Time: %02d:%02d:%02d Day# %d" strNL,
			test, xTimeGetDayName(sTM.tm_wday), sTM.tm_mday, xTimeGetMonthName(sTM.tm_mon), sTM.tm_year + YEAR_BASE_MIN,
			sTM.tm_hour, sTM.tm_min, sTM.tm_sec, sTM.tm_yday);

	sTSZ.usecs	= xTimeMakeTimeStamp(SECONDS_IN_EPOCH_FUTURE, 999999);
	snprintfx(test, sizeof(test), "%.6Z", &sTSZ);
	pcStringParseDateTime(test, &sTSZ.usecs, &sTM);
	PX("Input: %s => Date: %s %02d %s %04d Time: %02d:%02d:%02d Day# %d" strNL,
			test, xTimeGetDayName(sTM.tm_wday), sTM.tm_mday, xTimeGetMonthName(sTM.tm_mon), sTM.tm_year + YEAR_BASE_MIN,
			sTM.tm_hour, sTM.tm_min, sTM.tm_sec, sTM.tm_yday);
	#endif

	#if	(stringTEST_DATES)
	pcStringParseDateTime((char *) "2019/04/15", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "2019/04/15 ", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "2019/04/15T", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "2019/04/15t", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);

	pcStringParseDateTime((char *) "2019-04-15", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "2019-04-15 ", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "2019-04-15T", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "2019-04-15t", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	#endif

	#if	(stringTEST_TIMES)
	pcStringParseDateTime((char *) "1", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "1 ", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "01", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "01Z", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "1z", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);

	pcStringParseDateTime((char *) "1.1", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1100000 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "1.1 ", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1100000 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "1.1Z", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1100000 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "1.1z", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1100000 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);

	pcStringParseDateTime((char *) "1.000001", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1000001 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "1.000001 ", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1000001 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "1.000001Z", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1000001 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "1.000001z", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=0 || sTM.tm_mon!=0 || sTM.tm_mday!=0 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=1 || sTSZ.usecs != 1000001 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	#endif

	#if	(stringTEST_DTIME)
	pcStringParseDateTime((char *) "2019/04/15T1:23:45.678901Z", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=1 || sTM.tm_min!=23 || sTM.tm_sec!=45 || (sTSZ.usecs % MILLION) != 678901 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);

	pcStringParseDateTime((char *) "2019-04-15t01h23m45s678901z", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=1 || sTM.tm_min!=23 || sTM.tm_sec!=45 || (sTSZ.usecs % MILLION) != 678901 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);

	pcStringParseDateTime((char *) "2019/04-15 1:23m45.678901", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=1 || sTM.tm_min!=23 || sTM.tm_sec!=45 || (sTSZ.usecs % MILLION) != 678901 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);

	pcStringParseDateTime((char *) "2019-04/15t01h23:45s678901", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=49 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=1 || sTM.tm_min!=23 || sTM.tm_sec!=45 || (sTSZ.usecs % MILLION) != 678901 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	#endif

	#if	(stringTEST_RELDAT)
	pcStringParseDateTime((char *) "1/1-1", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=1 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	pcStringParseDateTime((char *) "1-1/1", &sTSZ.usecs, &sTM);
	PX(sTM.tm_year!=1 || sTM.tm_mon!=3 || sTM.tm_mday!=15 || sTM.tm_hour!=0 || sTM.tm_min!=0 || sTM.tm_sec!=0 ? " #%d Failed" strNL : " #%d Passed" strNL, __LINE__);
	#endif
}
