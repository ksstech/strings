/*
 * Copyright 2014-20 Andre M Maree / KSS Technologies (Pty) Ltd.
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
 * x_string_to_values.c
 */

#include	"x_definitions.h"
#include	"x_string_general.h"
#include	"x_string_to_values.h"
#include	"x_values_convert.h"
#include	"printfx.h"									// +x_definitions +stdarg +stdint +stdio
#include	"syslog.h"
#include	"x_errors_events.h"

#include	"hal_config.h"

#include	"lwip/def.h"

#include	<math.h>

#define	debugFLAG					0x4000

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
uint64_t char2u64(uint8_t * pSrc, uint64_t * pDst, uint32_t Len) {
	IF_myASSERT(debugPARAM, pSrc && INRANGE(1, Len, 8, uint32_t)) ;
	uint64_t	x64Val = 0ULL ;
	while (Len--) {
		x64Val <<= 8 ;
		x64Val += (uint64_t) *pSrc++ ;
	}
	if (pDst) {
		*pDst = x64Val ;
	}
	return x64Val ;
}

/**
 * xHexCharToValue() - Convert ASCI character to [hexa]decimal value, both upper and lower case input
 * @param cChr		ASCII data
 * @param xBase		base to handle 10/16
 * @return			if valid value, 0x00 -> 0x09 [0x0F] else -1
 */
int32_t	xHexCharToValue(uint8_t cChr, int32_t xBase) {
	if (INRANGE(CHR_0, cChr, CHR_9, uint8_t)) {
		return cChr - CHR_0 ;
	}
	if (xBase == BASE16) {
		if (INRANGE(CHR_A, cChr, CHR_F, uint8_t)) {
			return cChr - CHR_A + 10 ;
		}
		if (INRANGE(CHR_a, cChr, CHR_f, uint8_t)) {
			return cChr - CHR_a + 10 ;
		}
		if (cChr == CHR_O || cChr == CHR_o) {		// XXX TEMP fix for capture error
			SL_ERR("chr= 0x%x '%c'", cChr, cChr) ;
			return 0 ;
		}
	}
	return erFAILURE ;
}

uint64_t xStringParseX64(char *pSrc, uint8_t * pDst, uint32_t xLen) {
	IF_PRINT(debugPARSE_X64, "%.*s", xLen, pSrc) ;
	uint64_t xTemp = 0 ;
	uint8_t	x8Value = 0 ;
	int32_t iRV ;
	while (xLen && *pSrc) {
		iRV = xHexCharToValue(*pSrc, BASE16) ;
		if (iRV == erFAILURE) {						// invalid char
			SL_ERR("Invalid source Src=%s  Dst=%s", pSrc, pDst) ;
			break ;										// yes, stop parsing
		}
		x8Value += iRV ;							// nope, add to value
		if (xLen % 2) {									// odd length boundary?
			xTemp	<<= 8 ;
			xTemp += x8Value ;
			*pDst++ = x8Value ;							// store the value
			x8Value = 0 ;								// and reset for next 2 chars -> 2 nibbles
		} else {
			x8Value <<= 4 ;
		}
		++pSrc ;
		--xLen ;
	}
	return xTemp ;
}

/**
 * pcStringParseU64() - extract sign & DECIMAL value from source buffer and store at destinations
 * \brief		Assumes the string IS NULL TERMINATED
 * \brief		skips specified delimiters (if any)
 * \param[out]	pSign - pointer to destination for SIGN
 * \param[out]	pVal - pointer to destination for VALUE
 * \param[in]	pSrc - pointer to source buffer
 * \param[in]	pDel - pointer to string of valid delimiters
 * \param[in]	MaxLen - maximum number of characters in buffer
 * \return		updated pointer in source buffer to 1st non '0' -> '9' character
 * 				pcFAILURE is no valid value found to parse
 */
char *	pcStringParseU64(char * pSrc, uint64_t * pDst, int32_t * pSign, const char * pDel) {
	IF_myASSERT(debugPARAM, halCONFIG_inMEM(pSrc) && halCONFIG_inSRAM(pDst) && halCONFIG_inSRAM(pSign)) ;
	*pSign 	= 0 ;						// set sign as not provided
	uint64_t	Base = 10 ;				// default to decimal
	if (pDel) {
		IF_myASSERT(debugPARAM, halCONFIG_inMEM(pDel)) ;
		pSrc += xStringSkipDelim(pSrc, pDel, sizeof("+18,446,744,073,709,551,615")) ;
	}

	// check for sign at start
	if (*pSrc == CHR_MINUS) {			// NEGative sign?
		*pSign = -1 ;					// yes, flag accordingly
		++pSrc ;						// & skip over sign

	} else if (*pSrc == CHR_PLUS) {		// POSitive sign?
		*pSign = 1 ;					// yes, flag accordingly
		++pSrc ;						// & skip over sign

	} else if (*pSrc == CHR_X || *pSrc == CHR_x) {	// HEXadecimal format ?
		Base = 16 ;
		++pSrc ;						// & skip over hex format
	}

	// ensure something there to parse
	if (xHexCharToValue(*pSrc, Base) == erFAILURE) {
		return pcFAILURE ;
	}

	// now scan string and convert to value
	*pDst = 0ULL ;						// set default value as ZERO
	int32_t		Value ;
	while (*pSrc) {
		if ((Value = xHexCharToValue(*pSrc, Base)) == erFAILURE) {
			break ;
		}
		*pDst *= Base ;
		*pDst += Value ;
		++pSrc ;
	}
	return pSrc ;
}

/*
 * pcStringParseF64()
 * \brief		Read an double floating point value from the text buffer supplied
 * 				single leading '-' sign will be interpreted, before any number '1' - '9'
 *				leading 0's will be skipped
 *				First non-numeric character , other than '.', found AFTER a integer numeric will terminate the process
 *				First non-numeric character found AFTER a '.' (and valid numeric(s)) will terminate the process
 * \param		's' pointer to start of text buffer
 *				'd' is pointer to where double result must be stored
 * \return		pointer to 1st unprocessed (non numeric) char
 */
char *	pcStringParseF64(char *pSrc, double * pDst, int32_t * pSign, const char * pDel) {
	uint64_t	u64Val ;
	// parse the integer portion
	char * pTmp = pcStringParseU64(pSrc, &u64Val, pSign, " ") ;
	if (pTmp == pcFAILURE) {
		return pTmp ;
	}
	double dVal = u64Val ;
	u64Val = 0 ;
	int32_t	scale = 0 ;
	// handle fractional portion if decimal '.' & number follow
	if ((*pTmp == CHR_FULLSTOP) && (pTmp[1] >= CHR_0) && (pTmp[1] <= CHR_9)) {
		pTmp++ ;										// skip the '.'
		do {
			u64Val *= 10.0 ;							// adjust decimal scale
			u64Val += *pTmp++ - CHR_0 ;					// add fraction as part of integer (x10)
			scale-- ;									// adjust power to scale down with later
		} while (*pTmp >= CHR_0 && *pTmp <= CHR_9) ;	// do as long as numeric chars there
	}
	double dFrac	= u64Val ;
	dFrac	*= pow(10.0, scale) ;

	// handle exponent
	int32_t	subscale = 0 ;
	int32_t	signsubscale = 1 ;
	if ((*pTmp == CHR_e) || (*pTmp == CHR_E)) {			// exponent?
		pTmp++ ;										// yes, skip indicator
		if (*pTmp == CHR_PLUS) { 						// if positive
			pTmp++ ;									// skip
		} else {
			if (*pTmp == CHR_MINUS) {					// if negative
				signsubscale = -1 ;						// save the sign
				pTmp++ ;								// and skip over
			}
		}
		while (*pTmp >= CHR_0 && *pTmp <= CHR_9) {		// while we have numeric chars
			subscale = (subscale * 10) + (*pTmp++ - CHR_0) ;	// update the exponent value
		}
	}

	// calculate the actual value (number = +/- number.fraction * 10^+/- exponent)
	if (subscale) {
		*pDst = (dVal + dFrac) * pow(10.0 , (subscale * signsubscale)) ;
	} else {
		*pDst = dVal + dFrac ;
	}
	if (*pSign < 0) {
		*pDst *= -1.0 ;
	}
	IF_PRINT(debugPARSE_F64, "Input: %*s dInt=%f dFrac=%f Scale=%d SubScale=%d F64=%f\n", pTmp - pSrc, pSrc, dVal, dFrac, scale, subscale, *pDst) ;
	return pTmp ;
}

/**
 * pcStringParseX64() - parse x64 value from buffer
 * @param[in]	pSrc - pointer to the buffer containing the value
 * @param[out]	x64Ptr - pointer to the location where value is to be stored
 * @return		FAILPTR if no valid value or keyword found else updated pointer to next character to be parsed
 */
char *	pcStringParseX64(char * pSrc, x64_t * px64Val, varform_t VarForm, const char * pDel) {
	char *	ptr1 ;
	int32_t	Sign ;
	px_t	px ;
	px.px64	= px64Val ;
	if (VarForm == vfFXX) {
		ptr1	= pcStringParseF64(pSrc, px.pf64, &Sign, pDel) ;
	} else {
		ptr1	= pcStringParseU64(pSrc, px.pu64, &Sign, pDel) ;
	}
	EQ_RETURN(ptr1, pcFAILURE)

	// ensure NO SIGN is specified if unsigned is requested, and no error returned
	if (Sign && (VarForm == vfUXX)) {
		*px.pu64	= 0ULL ;
		SL_ERR("Uxx cannot have +/- sign") ;
		return pcFAILURE ;
	}
	if ((VarForm == vfIXX) && (Sign == -1)) {
		*px.pi64	*= Sign ;
	}
	return ptr1 ;
}

char *	pcStringParseValue(char * pSrc, px_t px, varform_t VarForm, varsize_t VarSize, const char * pDel) {
	// assume we might find a 64bit value, so plan accordingly
	x64_t	x64Val ;
	IF_PRINT(debugPARSE_VALUE, "'%.3s' ->", pSrc) ;
	char * ptr1	= pcStringParseX64(pSrc, &x64Val, VarForm, pDel) ;
	EQ_RETURN(ptr1, pcFAILURE)

	// if what we were asked to scan is less than 64bit in size, scale it up/down...
	if (VarSize < vs64B) {
		x64Val	= xValuesScaleX64(x64Val, VarForm, VarSize) ;
	}

	// store at destination based on size
	switch(VarSize) {
	case vs08B:	*px.pu8	= x64Val.x8[0].u8 ;		break ;
	case vs16B:	*px.pu16= x64Val.x16[0].u16 ;	break ;
	case vs32B:	*px.pu32= x64Val.x32[0].u32 ;	break ;
	case vs64B:	*px.pu64= x64Val.u64 ;			break ;
	default:	SL_ERR(debugAPPL_PLACE) ;
	}
	IF_EXEC_4(debugPARSE_VALUE, vValuesReportXxx, NULL, px, VarForm, VarSize) ;
	return ptr1 ;
}

/**
 * pcStringParseValueRange
 * @param pSrc		string buffer to parse from
 * @param px		pointer to oocation where value to be stored
 * @param VarForm	vfUXX / vfIXX / vfFXX / vfSXX
 * @param VarSize	vs08B / vs16B / vs32B / vs64B
 * @param pDel		valid delimiters to skip over /verify against
 * @param x32Lo		lower limit valid value
 * @param x32Hi		upper limit valid value
 * @return			pointer to next char to process or pcFAILURE
 */
char *	pcStringParseValueRange(char * pSrc, px_t px, varform_t VarForm, varsize_t VarSize, const char * pDel, x32_t x32Lo, x32_t x32Hi) {
	x64_t	x64Val ;
	char * pTmp	= pcStringParseX64(pSrc, &x64Val, VarForm, pDel) ;
	EQ_RETURN(pTmp, pcFAILURE)

	// Lo & Hi values MUST be full 32bit width..
	x64_t x64Lo = xValuesUpscaleX32_X64(x32Lo, VarForm) ;
	x64_t x64Hi = xValuesUpscaleX32_X64(x32Hi, VarForm) ;
	IF_PRINT(debugPARSE_VALUE, " '%.*s'", pTmp - pSrc, pSrc) ;
	IF_EXEC_4(debugPARSE_VALUE, xCV_ReportValue, " Val=", x64Val, VarForm, VarSize) ;
	IF_EXEC_4(debugPARSE_VALUE, xCV_ReportValue, " Lo=", x64Lo, VarForm, VarSize) ;
	IF_EXEC_4(debugPARSE_VALUE, xCV_ReportValue, " Hi=", x64Hi, VarForm, VarSize) ;
	switch(VarForm) {
	case vfUXX:
		if ((x64Val.u64 < x64Lo.u64) || (x64Val.u64 > x64Hi.u64)) {
			return pcFAILURE ;
		}
		break ;
	case vfIXX:
		if ((x64Val.i64 < x64Lo.i64) || (x64Val.i64 > x64Hi.i64)) {
			return pcFAILURE ;
		}
		break ;
	case vfFXX:
		if ((x64Val.f64 < x64Lo.f64) || (x64Val.f64 > x64Hi.f64)) {
			return pcFAILURE ;
		}
		break ;
	case vfSXX:
	default:
		SL_ERR(debugAPPL_PLACE) ;
		return pcFAILURE ;
	}
	vValuesStoreX64_Xxx(x64Val, px, VarForm, VarSize) ;
	return pTmp ;
}

char *	pcStringParseValues(char * pSrc, px_t px, varform_t VarForm, varsize_t VarSize, const char * pDel, int32_t Count) {
	while(Count--) {
		char * ptr1	= pcStringParseValue(pSrc, px, VarForm, VarSize, pDel) ;
		EQ_RETURN(ptr1, pcFAILURE)
		switch(VarSize) {								// adjust the pointer based on the size of the destination storage
		case vs08B:	++px.px8 ;			break ;
		case vs16B:	++px.px16 ;			break ;
		case vs32B:	++px.px32 ;			break ;
		case vs64B:	++px.px64 ;			break ;
		default:	SL_ERR(debugAPPL_PLACE) ;
		}
		pSrc	= ptr1 ;								// set starting pointer ready for the next
	}
	return pSrc ;
}

char *	pcStringParseNumber(char * pSrc, px_t px) {
	char * pTmp = pSrc ;
	*px.pi32 = 0 ;
	while (*pSrc && INRANGE(CHR_0, *pSrc, CHR_9, int32_t)) {
		*px.pi32	*= 10 ;
		*px.pi32	+= *pSrc++ - CHR_0 ;
	}
	return (pTmp == pSrc) ? pcFAILURE : pSrc ;
}

char *	pcStringParseNumberRange(char * pSrc, px_t px, int32_t Min, int32_t Max) {
	pSrc = pcStringParseNumber(pSrc, px) ;
	return (INRANGE(Min, *px.pi32, Max, int32_t) == 1) ? pSrc : pcFAILURE ;
}

/**
 * pcStringParseIpAddr() - parse a string and return an IP address in NETWORK byte order
 * @param	pStr
 * @param	pVal
 * @return	pcFAILURE or pointer to 1st char after the IP address
 */
char *	pcStringParseIpAddr(char * pSrc, px_t px) {
	IF_myASSERT(debugPARAM, halCONFIG_inMEM(pSrc) && halCONFIG_inSRAM(px.pu32)) ;
	char * pcRV = pSrc ;
	*px.pu32 = 0 ;
	for(int32_t i = 0; i < 4; ++i) {
		uint32_t u32Val = 0 ;
		const char * pccTmp = (i == 0) ? " " : "." ;
		pcRV = pcStringParseValueRange(pSrc = pcRV, (px_t) &u32Val, vfUXX, vs32B, pccTmp, (x32_t) UINT8_MIN, (x32_t) UINT8_MAX) ;
		EQ_GOTO(pcRV, pcFAILURE, exit) ;
//		PRINT("i=%d  '%s' -> '%s'  0x%08x  %-I  ", i, pSrc, pcRV, u32Val, u32Val) ;
		*px.pu32	<<= 8 ;
		*px.pu32	+= u32Val ;
		if (*pcRV == CHR_FULLSTOP) {
			++pcRV ;
		}
	}
	*px.pu32	= htonl(*px.pu32) ;
	IF_PRINT(debugRESULT, "IP : %#-I\n", *px.pu32) ;
exit:
	return pcRV ;
}

void	x_string_values_test(void) {
	x64_t	x64Val ;
	// Test error if sign present for vfUXX
	SL_INFO("Sign test") ;
	pcStringParseValue((char *) "123456", (px_t) &x64Val, vfUXX, vs08B, " ") ;
	pcStringParseValue((char *) "+123456", (px_t) &x64Val, vfUXX, vs16B, " ") ;
	pcStringParseValue((char *) "-123456", (px_t) &x64Val, vfUXX, vs32B, " ") ;
	pcStringParseValue((char *) "123456", (px_t) &x64Val, vfUXX, vs64B, " ") ;
	// Unsigned round down
	SL_INFO("U-8/16/32 test") ;
	pcStringParseValue((char *) "257", (px_t) &x64Val, vfUXX, vs08B, " ") ;
	pcStringParseValue((char *) "65537", (px_t) &x64Val, vfUXX, vs16B, " ") ;
	pcStringParseValue((char *) "4294967297", (px_t) &x64Val, vfUXX, vs32B, " ") ;
	// Signed round down
	SL_INFO("I8 signed test") ;
	pcStringParseValue((char *) "130", (px_t) &x64Val, vfIXX, vs08B, " ") ;
	pcStringParseValue((char *) "-130", (px_t) &x64Val, vfIXX, vs08B, " ") ;
	pcStringParseValue((char *) "+130", (px_t) &x64Val, vfIXX, vs08B, " ") ;
	SL_INFO("I16 signed test") ;
	pcStringParseValue((char *) "32770", (px_t) &x64Val, vfIXX, vs16B, " ") ;
	pcStringParseValue((char *) "+32770", (px_t) &x64Val, vfIXX, vs16B, " ") ;
	pcStringParseValue((char *) "-32770", (px_t) &x64Val, vfIXX, vs16B, " ") ;
	SL_INFO("I32 signed test") ;
	pcStringParseValue((char *) "2147483660", (px_t) &x64Val, vfIXX, vs32B, " ") ;
	pcStringParseValue((char *) "+2147483660", (px_t) &x64Val, vfIXX, vs32B, " ") ;
	pcStringParseValue((char *) "-2147483660", (px_t) &x64Val, vfIXX, vs32B, " ") ;
	// Float round down
	SL_INFO("F32 test") ;
	pcStringParseValue((char *) "1.1", (px_t) &x64Val, vfFXX, vs32B, " ") ;
	pcStringParseValue((char *) "123.123", (px_t) &x64Val, vfFXX, vs32B, " ") ;
	pcStringParseValue((char *) "123456.123456", (px_t) &x64Val, vfFXX, vs32B, " ") ;
	pcStringParseValue((char *) "123456789.123456", (px_t) &x64Val, vfFXX, vs32B, " ") ;
	pcStringParseValue((char *) "1234567890.1234567", (px_t) &x64Val, vfFXX, vs32B, " ") ;
	pcStringParseValue((char *) "+1234567890.1234567", (px_t) &x64Val, vfFXX, vs32B, " ") ;
	pcStringParseValue((char *) "-1234567890.1234567", (px_t) &x64Val, vfFXX, vs32B, " ") ;
	// Double no round
	SL_INFO("F64 test") ;
	pcStringParseValue((char *) "12345678901234567.123456789", (px_t) &x64Val, vfFXX, vs64B, " ") ;
	// Version format
	SL_INFO("4x U8 test") ;
	pcStringParseValues((char *) "11.22.33.44", (px_t) &x64Val, vfUXX, vs08B, " .", 4) ;
}
