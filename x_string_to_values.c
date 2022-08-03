/*
 * x_string_to_values.c
 * Copyright 2014-22 (c) Andre M. Maree / KSS Technologies (Pty) Ltd.
 */

#include	<netinet/in.h>
#include	<math.h>

#include	"hal_config.h"
#include	"x_string_general.h"
#include	"x_string_to_values.h"
#include	"printfx.h"									// +x_definitions +stdarg +stdint +stdio
#include	"syslog.h"
#include	"x_errors_events.h"

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
char * pcStringParseU64(char * pSrc, u64_t * pDst, int * pSign, const char * pDel) {
	IF_myASSERT(debugPARAM, halCONFIG_inMEM(pSrc) && halCONFIG_inSRAM(pDst) && halCONFIG_inSRAM(pSign)) ;
	IF_myASSERT(debugPARAM && pDel, halCONFIG_inMEM(pDel)) ;
	u64_t Base = 10 ;								// default to decimal
	*pSign 	= 0 ;										// set sign as not provided
	if (pDel)
		pSrc += xStringSkipDelim(pSrc, pDel, sizeof("+18,446,744,073,709,551,615"));
	if (*pSrc == CHR_MINUS) {							// NEGative sign?
		*pSign = -1 ;									// yes, flag accordingly
		++pSrc ;										// & skip over sign
	} else if (*pSrc == CHR_PLUS) {						// POSitive sign?
		*pSign = 1 ;									// yes, flag accordingly
		++pSrc ;										// & skip over sign
	} else if (*pSrc == CHR_X || *pSrc == CHR_x) {		// HEXadecimal format ?
		Base = 16 ;
		++pSrc ;										// & skip over hex format
	}
	if (xHexCharToValue(*pSrc, Base) == erFAILURE)		// ensure something there to parse
		return pcFAILURE;
	*pDst = 0ULL ;					// set default value as ZERO
	int	Value ;
	while (*pSrc) {					// now scan string and convert to value
		if ((Value = xHexCharToValue(*pSrc, Base)) == erFAILURE)
			break;
		*pDst *= Base ;
		*pDst += Value ;
		++pSrc ;
	}
	IF_P(debugPARSE_VALUE, " %llu / %lld / 0x%llx", *pDst, *pDst * *pSign, *pDst, *pDst) ;
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
char * pcStringParseF64(char *pSrc, double * pDst, int * pSign, const char * pDel) {
	u64_t u64Val ;
	// parse the integer portion
	char * pTmp = pcStringParseU64(pSrc, &u64Val, pSign, " ") ;
	if (pTmp == pcFAILURE)
		return pTmp ;
	double dVal = u64Val ;
	u64Val = 0 ;
	int	scale = 0 ;
	// handle fractional portion if decimal '.' & number follow
	if ((*pTmp == CHR_FULLSTOP) && (pTmp[1] >= CHR_0) && (pTmp[1] <= CHR_9)) {
		pTmp++ ;										// skip the '.'
		do {
			u64Val *= 10.0 ;							// adjust decimal scale
			u64Val += *pTmp++ - CHR_0;					// add fraction as part of integer (x10)
			scale-- ;									// adjust power to scale down with later
		} while (*pTmp >= CHR_0 && *pTmp <= CHR_9);		// do as long as numeric chars there
	}
	double dFrac = u64Val ;
	dFrac *= pow(10.0, scale) ;

	// handle exponent
	int	subscale = 0 ;
	int	signsubscale = 1 ;
	if ((*pTmp == CHR_E) || (*pTmp == CHR_e)) {			// exponent?
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
			subscale = (subscale * 10) + (*pTmp++ - CHR_0);	// update the exponent value
		}
	}

	// calculate the actual value (number = +/- number.fraction * 10^+/- exponent)
	if (subscale)
		*pDst = (dVal + dFrac) * pow(10.0 , (subscale * signsubscale)) ;
	else
		*pDst = dVal + dFrac ;
	if (*pSign < 0)
		*pDst *= -1.0 ;
	IF_P(debugPARSE_F64, "Input: %*s dInt=%f dFrac=%f Scale=%d SubScale=%d F64=%f\r\n", pTmp - pSrc, pSrc, dVal, dFrac, scale, subscale, *pDst) ;
	return pTmp ;
}

/**
 * pcStringParseX64() - parse x64 value from buffer
 * @param[in]	pSrc - pointer to the buffer containing the value
 * @param[out]	x64Ptr - pointer to the location where value is to be stored
 * @return		FAILPTR if no valid value or keyword found else updated pointer to next character to be parsed
 */
char * pcStringParseX64(char * pSrc, x64_t * px64Val, vf_e cvF, const char * pDel) {
	char *	pTmp ;
	int	Sign ;
	px_t pX ;
	pX.px64	= px64Val ;
	if (cvF == vfFXX)
		pTmp = pcStringParseF64(pSrc, pX.pf64, &Sign, pDel) ;
	else
		pTmp = pcStringParseU64(pSrc, pX.pu64, &Sign, pDel) ;
	EQ_RETURN(pTmp, pcFAILURE)

	// ensure NO SIGN is specified if unsigned is requested, and no error returned
	if (Sign && (cvF == vfUXX)) {
		*pX.pu64	= 0ULL ;
		P("  Uxx cannot have +/- sign") ;
		return pcFAILURE ;
	}
	if ((cvF == vfIXX) && (Sign == -1))
		*pX.pi64 *= Sign ;
	return pTmp ;
}

char * pcStringParseValue(char * pSrc, px_t pX, vf_e cvF, vs_e cvS, const char * pDel) {
	x64_t x64Val;
	IF_P(debugPARSE_VALUE, "'%.3s' ->", pSrc);
	char * pTmp	= pcStringParseX64(pSrc, &x64Val, cvF, pDel);
	if (pTmp != pcFAILURE)
		vValuesStoreX64_Xxx(x64Val, pX, cvF, cvS);
	return pTmp;
}

char * pcStringParseParam(char * pSrc, px_t pX, cvi_e cvI) {
	return pcStringParseValue(pSrc, pX, xCV_Index2Form(cvI), xCV_Index2Size(cvI), sepSPACE_COMMA) ;
}

/**
 * pcStringParseValueRange
 * @param pSrc		string buffer to parse from
 * @param pX		pointer to location where value to be stored
 * @param cvF	vfUXX / vfIXX / vfFXX / vfSXX
 * @param cvS	vs08B / vs16B / vs32B / vs64B
 * @param pDel		valid delimiters to skip over /verify against
 * @param x32Lo		lower limit valid value
 * @param x32Hi		upper limit valid value
 * @return			pointer to next char to process or pcFAILURE
 */
char * pcStringParseValueRange(char * pSrc, px_t pX, vf_e cvF, vs_e cvS, const char * pDel, x32_t x32Lo, x32_t x32Hi) {
	char * pTmp = pcStringParseValue(pSrc, pX, cvF, cvS, pDel);
	if (pTmp != pcFAILURE) {
		x64_t x64Val= xValuesUpscaleXxx_X64(pX, cvF, cvS) ;
		x64_t x64Lo = xValuesUpscaleXxx_X64((px_t) &x32Lo, cvF, vs32B) ;
		x64_t x64Hi = xValuesUpscaleXxx_X64((px_t) &x32Hi, cvF, vs32B) ;
		IF_P(debugPARSE_VALUE, "  '%.*s'", pTmp - pSrc, pSrc) ;
		IF_EXEC_4(debugPARSE_VALUE, xCV_ReportValue, "  Val=", x64Val, cvF, cvS) ;
		IF_EXEC_4(debugPARSE_VALUE, xCV_ReportValue, "  Lo=", x64Lo, cvF, cvS) ;
		IF_EXEC_4(debugPARSE_VALUE, xCV_ReportValue, "  Hi=", x64Hi, cvF, cvS) ;
		switch(cvF) {
		case vfUXX:
			if ((x64Val.u64 < x64Lo.u64) || (x64Val.u64 > x64Hi.u64))
				return pcFAILURE ;
			break ;
		case vfIXX:
			if ((x64Val.i64 < x64Lo.i64) || (x64Val.i64 > x64Hi.i64))
				return pcFAILURE ;
			break ;
		case vfFXX:
			if ((x64Val.f64 < x64Lo.f64) || (x64Val.f64 > x64Hi.f64))
				return pcFAILURE ;
			break ;
		case vfSXX:
			IF_myASSERT(debugPARAM, 0);
			return pcFAILURE ;
		}
		IF_P(debugPARSE_VALUE, "\r\n");
		vValuesStoreX64_Xxx(x64Val, pX, cvF, cvS);
	}
	return pTmp ;
}

char * pcStringParseValues(char * pSrc, px_t pX, vf_e cvF, vs_e cvS, const char * pDel, int Count) {
	while(Count--) {
		char * pTmp	= pcStringParseValue(pSrc, pX, cvF, cvS, pDel) ;
		EQ_RETURN(pTmp, pcFAILURE);
		pX = pxCV_AddressNextWithSize(pX, cvS);
		pSrc	= pTmp ;								// set starting pointer ready for the next
	}
	return pSrc ;
}

char * pcStringParseNumber(char * pSrc, px_t pX) {
	char * pTmp = pSrc ;
	*pX.pi32 = 0 ;
	while (*pSrc && INRANGE(CHR_0, *pSrc, CHR_9)) {
		*pX.pi32	*= 10 ;
		*pX.pi32	+= *pSrc++ - CHR_0;
	}
	return (pTmp == pSrc) ? pcFAILURE : pSrc ;
}

char * pcStringParseNumberRange(char * pSrc, px_t pX, int Min, int Max) {
	pSrc = pcStringParseNumber(pSrc, pX) ;
	return INRANGE(Min, *pX.pi32, Max) ? pSrc : pcFAILURE;
}

/**
 * pcStringParseIpAddr() - parse a string and return an IP address in NETWORK byte order
 * @param	pStr
 * @param	pVal
 * @return	pcFAILURE or pointer to 1st char after the IP address
 */
char * pcStringParseIpAddr(char * pSrc, px_t pX) {
	IF_myASSERT(debugPARAM, halCONFIG_inMEM(pSrc) && halCONFIG_inSRAM(pX.pu32)) ;
	char * pcRV = (char *)pSrc ;
	*pX.pu32 = 0 ;
	for(int i = 0; i < 4; ++i) {
		u32_t u32Val = 0 ;
		const char * pccTmp = (i == 0) ? " " : "." ;
		pcRV = pcStringParseValueRange(pSrc = pcRV, (px_t) &u32Val, vfUXX, vs32B, pccTmp, (x32_t) 0, (x32_t) 255) ;
		if (pcRV == pcFAILURE)
			return pcFAILURE ;
		*pX.pu32	<<= 8 ;
		*pX.pu32	+= u32Val ;
		if (*pcRV == CHR_FULLSTOP)
			++pcRV;
	}
	*pX.pu32 = htonl(*pX.pu32) ;
	IF_P(debugRESULT, "IP : %#-I\r\n", *pX.pu32) ;
	return pcRV ;
}

void x_string_values_test(void) {
	x64_t x64Val ;
	// Test error if sign present for vfUXX
	P("Sign test\r\n") ;
	pcStringParseValue((char *) "123456", (px_t) &x64Val, vfUXX, vs08B, " ") ;
	pcStringParseValue((char *) "+123456", (px_t) &x64Val, vfUXX, vs16B, " ") ;
	pcStringParseValue((char *) "-123456", (px_t) &x64Val, vfUXX, vs32B, " ") ;
	pcStringParseValue((char *) "123456", (px_t) &x64Val, vfUXX, vs64B, " ") ;
	// Unsigned round down
	P("U-8/16/32 test\r\n") ;
	pcStringParseValue((char *) "257", (px_t) &x64Val, vfUXX, vs08B, " ") ;
	pcStringParseValue((char *) "65537", (px_t) &x64Val, vfUXX, vs16B, " ") ;
	pcStringParseValue((char *) "4294967297", (px_t) &x64Val, vfUXX, vs32B, " ") ;
	// Signed round down
	P("I8 signed test\r\n") ;
	pcStringParseValue((char *) "130", (px_t) &x64Val, vfIXX, vs08B, " ") ;
	pcStringParseValue((char *) "-130", (px_t) &x64Val, vfIXX, vs08B, " ") ;
	pcStringParseValue((char *) "+130", (px_t) &x64Val, vfIXX, vs08B, " ") ;
	P("I16 signed test\r\n") ;
	pcStringParseValue((char *) "32770", (px_t) &x64Val, vfIXX, vs16B, " ") ;
	pcStringParseValue((char *) "+32770", (px_t) &x64Val, vfIXX, vs16B, " ") ;
	pcStringParseValue((char *) "-32770", (px_t) &x64Val, vfIXX, vs16B, " ") ;
	P("I32 signed test\r\n") ;
	pcStringParseValue((char *) "2147483660", (px_t) &x64Val, vfIXX, vs32B, " ") ;
	pcStringParseValue((char *) "+2147483660", (px_t) &x64Val, vfIXX, vs32B, " ") ;
	pcStringParseValue((char *) "-2147483660", (px_t) &x64Val, vfIXX, vs32B, " ") ;
	// Float round down
	P("F32 test\r\n") ;
	pcStringParseValue((char *) "1.1", (px_t) &x64Val, vfFXX, vs32B, " ") ;
	pcStringParseValue((char *) "123.123", (px_t) &x64Val, vfFXX, vs32B, " ") ;
	pcStringParseValue((char *) "123456.123456", (px_t) &x64Val, vfFXX, vs32B, " ") ;
	pcStringParseValue((char *) "123456789.123456", (px_t) &x64Val, vfFXX, vs32B, " ") ;
	pcStringParseValue((char *) "1234567890.1234567", (px_t) &x64Val, vfFXX, vs32B, " ") ;
	pcStringParseValue((char *) "+1234567890.1234567", (px_t) &x64Val, vfFXX, vs32B, " ") ;
	pcStringParseValue((char *) "-1234567890.1234567", (px_t) &x64Val, vfFXX, vs32B, " ") ;
	// Double no round
	P("F64 test\r\n") ;
	pcStringParseValue((char *) "12345678901234567.123456789", (px_t) &x64Val, vfFXX, vs64B, " ") ;
	// Version format
	P("4x U8 test\r\n") ;
	pcStringParseValues((char *) "11.22.33.44", (px_t) &x64Val, vfUXX, vs08B, " .", 4) ;
}
