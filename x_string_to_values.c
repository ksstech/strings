/*
 * x_string_to_values.c
 * Copyright 2014-22 (c) Andre M. Maree / KSS Technologies (Pty) Ltd.
 */

#include <netinet/in.h>
#include <math.h>

#include "hal_variables.h"
#include "x_string_general.h"
#include "x_string_to_values.h"
#include "printfx.h"									// +x_definitions +stdarg +stdint +stdio
#include "syslog.h"
#include "x_errors_events.h"

#define	debugFLAG					0x6000

#define	debugPARSE_U64				(debugFLAG & 0x0001)
#define	debugPARSE_F64				(debugFLAG & 0x0002)
#define	debugPARSE_X64				(debugFLAG & 0x0004)
#define	debugPARSE_VALUE			(debugFLAG & 0x0008)

#define	debugTIMING					(debugFLAG_GLOBAL & debugFLAG & 0x1000)
#define	debugTRACK					(debugFLAG_GLOBAL & debugFLAG & 0x2000)
#define	debugPARAM					(debugFLAG_GLOBAL & debugFLAG & 0x4000)
#define	debugRESULT					(debugFLAG_GLOBAL & debugFLAG & 0x8000)

/*
 * char2u64 - Convert n chars to a unsigned 64 bit value
 * \param[in]	pointer to	string of chars to be read and converted
 * \param[in]	pointer to 64 bit location where result to be stored.
 * \param[in]	number of chars to read and convert
 *
 * \return		converted value
 *
 * \brief		If ASSERT not defined, no range checking done on parameters
 * 				If only converted value required as return, *d can be made NULL ;
 */
u64_t char2u64(char * pSrc, u64_t * pDst, int Len) {
	IF_myASSERT(debugPARAM, pSrc && INRANGE(1, Len, 8));
	u64_t x64Val = 0ULL ;
	while (Len--) {
		x64Val <<= 8 ;
		x64Val += (u64_t) *pSrc++ ;
	}
	if (pDst) *pDst = x64Val ;
	return x64Val ;
}

/**
 * xHexCharToValue() - Convert ASCI character to [hexa]decimal value, both upper and lower case input
 * @param cChr		ASCII data
 * @param xBase		base to handle 10/16
 * @return			if valid value, 0x00 -> 0x09 [0x0F] else -1
 */
int	xHexCharToValue(char cChr, int xBase) {
	if (INRANGE(CHR_0, cChr, CHR_9))
		return cChr - CHR_0;
	if (xBase == BASE16) {
		if (INRANGE(CHR_A, cChr, CHR_F))
			return cChr - CHR_A + 10;
		if (INRANGE(CHR_a, cChr, CHR_f))
			return cChr - CHR_a + 10;
		if (cChr == CHR_O || cChr == CHR_o) {			// XXX TEMP fix for capture error
			P("chr= 0x%x '%c'", cChr, cChr);
			return 0;
		}
	}
	return erFAILURE ;
}

u64_t xStringParseX64(char *pSrc, char * pDst, int xLen) {
	IF_P(debugPARSE_X64, "%.*s", xLen, pSrc);
	u64_t xTemp = 0;
	u8_t x8Value = 0;
	int iRV;
	while (xLen && *pSrc) {
		iRV = xHexCharToValue(*pSrc, BASE16);
		if (iRV == erFAILURE) {							// invalid char
			SL_ERR("Invalid source Src=%s  Dst=%s", pSrc, pDst) ;
			break;										// yes, stop parsing
		}
		x8Value += iRV;									// nope, add to value
		if (xLen % 2) {									// odd length boundary?
			xTemp <<= 8;
			xTemp += x8Value;
			*pDst++ = x8Value;							// store the value
			x8Value = 0;								// and reset for next 2 chars -> 2 nibbles
		} else {
			x8Value <<= 4;
		}
		++pSrc;
		--xLen;
	}
	return xTemp;
}

/**
/**
 * pcStringParseIpAddr() - parse a string and return an IP address in NETWORK byte order
 * @param	pStr
 * @param	pVal
 * @return	pcFAILURE or pointer to 1st char after the IP address
 */
char * pcStringParseIpAddr(char * pSrc, px_t pX) {
}
