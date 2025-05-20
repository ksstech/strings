// string_general.c - Copyright (c) 2014-25 Andre M. Maree / KSS Technologies (Pty) Ltd.

#include "hal_platform.h"
#include "hal_memory.h"
#include "printfx.h"
#include "hal_options.h"
#include "errors_events.h"
#include "string_general.h"
#include "string_to_values.h"
#include "common-vars.h"

#include <string.h>
#include <ctype.h>

// ########################################### Macros ##############################################

#define	debugFLAG					0xF000
#define	debugDELIM					(debugFLAG & 0x0200)
#define	debugTIMING					(debugFLAG_GLOBAL & debugFLAG & 0x1000)
#define	debugTRACK					(debugFLAG_GLOBAL & debugFLAG & 0x2000)
#define	debugPARAM					(debugFLAG_GLOBAL & debugFLAG & 0x4000)
#define	debugRESULT					(debugFLAG_GLOBAL & debugFLAG & 0x8000)

#ifndef stringMAX_LEN
	#define	stringMAX_LEN			2048
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

// #################################################################################################

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
