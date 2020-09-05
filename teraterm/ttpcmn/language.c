/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2020 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// TTCMN.DLL character code conversion

#include <mbstring.h>
#include <locale.h>
#include "teraterm.h"
#include "tttypes.h"
#include "codemap.h"

#define DllExport __declspec(dllexport)
#include "language.h"

// exportされている
DllExport unsigned short ConvertUnicode(unsigned short code, const codemap_t *table, int tmax)
{
	int low, mid, high;
	unsigned short result;

	low = 0;
	high = tmax - 1;
	result = 0; // convert error

	// binary search
	while (low < high) {
		mid = (low + high) / 2;
		if (table[mid].from_code < code) {
			low = mid + 1;
		} else {
			high = mid;
		}
	}

	if (table[low].from_code == code) {
		result = table[low].to_code;
	}

	return (result);
}

// Japanese SJIS -> JIS
DllExport WORD PASCAL SJIS2JIS(WORD KCode)
{
	WORD x0,x1,x2,y0;
	BYTE b = LOBYTE(KCode);

	if ((b>=0x40) && (b<=0x7f)) {
		x0 = 0x8140;
		y0 = 0x2121;
	}
	else if ((b>=0x80) && (b<=0x9e)) {
		x0 = 0x8180;
		y0 = 0x2160;
	}
	else {
		x0 = 0x819f;
		y0 = 0x2221;
	}
	if (HIBYTE(KCode) >= 0xe0) {
		x0 = x0 + 0x5f00;
		y0 = y0 + 0x3e00;
	}
	x1 = (KCode-x0) / 0x100;
	x2 = (KCode-x0) % 0x100;
	return (y0 + x1*0x200 + x2);
}

// Japanese SJIS -> EUC
DllExport WORD PASCAL SJIS2EUC(WORD KCode)
{
	return (SJIS2JIS(KCode) | 0x8080);
}

// Japanese JIS -> SJIS
DllExport WORD PASCAL JIS2SJIS(WORD KCode)
{
	WORD n1, n2, SJIS;

	n1 = (KCode-0x2121) / 0x200;
	n2 = (KCode-0x2121) % 0x200;

	if (n1<=0x1e) {
		SJIS = 0x8100 + n1*256;
	}
	else {
		SJIS = 0xC100 + n1*256;
	}

	if (n2<=0x3e) {
		return (SJIS + n2 + 0x40);
	}
	else if ((n2>=0x3f) && (n2<=0x5d)) {
		return (SJIS + n2 + 0x41);
	}
	else {
		return (SJIS + n2 - 0x61);
	}
}

/* Russian charset conversion table by Andrey Nikiforov 19971114 */
/* Updated by NAGATA Shinya 20111228 */
//   変換先に対応する文字・記号がないところをすぐ下の表で示す
//     該当部分は重複がないように割り当てる(オリジナルがそうなっているので)
static const BYTE cpconv[4][4][128] =
{
	{
		{
			// 1251 -> 1251 = dummy
			/*128-143*/  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			/*144-159*/  144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
			/*160-175*/  160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
			/*176-191*/  176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
			/*192-207*/  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
			/*208-223*/  208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
			/*224-239*/  224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
			/*240-255*/  240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
		},
		{
			// 1251 -> KOI8-R
			/*128-143*/  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			/*144-159*/  144,145,146,147,148,149,150,151,152,153,155,157,159,160,161,162,
			/*160-175*/  154,164,165,166,167,168,169,170,179,191,171,172,173,174,175,176,
			/*176-191*/  156,177,178,180,181,182,183,158,163,184,185,186,187,188,189,190,
			/*192-207*/  225,226,247,231,228,229,246,250,233,234,235,236,237,238,239,240,
			/*208-223*/  242,243,244,245,230,232,227,254,251,253,255,249,248,252,224,241,
			/*224-239*/  193,194,215,199,196,197,214,218,201,202,203,204,205,206,207,208,
			/*240-255*/  210,211,212,213,198,200,195,222,219,221,223,217,216,220,192,209,
			//   Non mapped character
			//128-143    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			//144-159    144,145,146,147,148,149,150,151,152,153,155,157,159,160,161,162,
			//160-175       ,164,165,166,167,168,169,170,   ,   ,171,172,173,174,175,176,
			//176-191       ,177,178,180,181,182,183,   ,   ,184,185,186,187,188,189,190,
			//192-207       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//208-223       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//224-239       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//240-255       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
		},
		{
			// 1251 -> 866
			/*128-143*/  176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
			/*144-159*/  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
			/*160-175*/  255,246,247,208,209,210,211,212,240,213,242,214,215,216,217,244,
			/*176-191*/  248,218,219,220,221,222,223,250,241,252,243,249,251,253,254,245,
			/*192-207*/  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			/*208-223*/  144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
			/*224-239*/  160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
			/*240-255*/  224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
			//   Non mapped character
			//128-143    176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
			//144-159    192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
			//160-175       ,   ,   ,208,209,210,211,212,   ,213,   ,214,215,216,217,   ,
			//176-191       ,218,219,220,221,222,223,   ,   ,   ,   ,249,251,253,254,   ,
			//192-207       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//208-223       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//224-239       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//240-255       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
		},
		{
			// 1251 -> ISO
			/*128-143*/  162,163,128,243,129,130,131,132,133,134,169,135,170,172,171,175,
			/*144-159*/  242,136,137,138,139,140,141,142,143,144,249,145,250,252,251,255,
			/*160-175*/  160,174,254,168,146,147,148,253,161,149,164,150,151,173,152,167,
			/*176-191*/  153,154,166,246,155,156,157,158,241,240,244,159,248,165,245,247,
			/*192-207*/  176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
			/*208-223*/  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
			/*224-239*/  208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
			/*240-255*/  224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
			//   Non mapped character
			//128-143       ,   ,128,   ,129,130,131,132,133,134,   ,135,   ,   ,   ,   ,
			//144-159       ,136,137,138,139,140,141,142,143,144,   ,145,   ,   ,   ,   ,
			//160-175       ,   ,   ,   ,146,147,148,   ,   ,149,   ,150,151,   ,152,   ,
			//176-191    153,154,   ,   ,155,156,157,158,   ,   ,   ,159,   ,   ,   ,   ,
			//192-207       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//208-223       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//224-239       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//240-255       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
		},
	},
	{
		{
			// koi8-r -> 1251
			/*128-143*/  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			/*144-159*/  144,145,146,147,148,149,150,151,152,153,160,154,176,155,183,156,
			/*160-175*/  157,158,159,184,161,162,163,164,165,166,167,170,171,172,173,174,
			/*176-191*/  175,177,178,168,179,180,181,182,185,186,187,188,189,190,191,169,
			/*192-207*/  254,224,225,246,228,229,244,227,245,232,233,234,235,236,237,238,
			/*208-223*/  239,255,240,241,242,243,230,226,252,251,231,248,253,249,247,250,
			/*224-239*/  222,192,193,214,196,197,212,195,213,200,201,202,203,204,205,206,
			/*240-255*/  207,223,208,209,210,211,198,194,220,219,199,216,221,217,215,218,
			//   Non mapped character
			//128-143    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			//144-159    144,145,146,147,148,149,150,151,152,153,   ,154,   ,155,   ,156,
			//160-175    157,158,159,   ,161,162,163,164,165,166,167,170,171,172,173,174,
			//176-191    175,177,178,   ,179,180,181,182,185,186,187,188,189,190,191,   ,
			//192-207       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//208-223       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//224-239       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//240-255       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
		},
		{
			// koi8-r -> koi8-r = dummy
			/*128-143*/  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			/*144-159*/  144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
			/*160-175*/  160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
			/*176-191*/  176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
			/*192-207*/  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
			/*208-223*/  208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
			/*224-239*/  224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
			/*240-255*/  240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
		},
		{
			// koi8-r -> 866
			/*128-143*/  196,179,218,191,192,217,195,180,194,193,197,223,220,219,221,222,
			/*144-159*/  176,177,178,242,254,249,251,243,244,245,255,246,248,247,250,252,
			/*160-175*/  205,186,213,241,214,201,184,183,187,212,211,200,190,189,188,198,
			/*176-191*/  199,204,181,240,182,185,209,210,203,207,208,202,216,215,206,255,
			/*192-207*/  238,160,161,230,164,165,228,163,229,168,169,170,171,172,173,174,
			/*208-223*/  175,239,224,225,226,227,166,162,236,235,167,232,237,233,231,234,
			/*224-239*/  158,128,129,150,132,133,148,131,149,136,137,138,139,140,141,142,
			/*240-255*/  143,159,144,145,146,147,134,130,156,155,135,152,157,153,151,154,
			//   Non mapped character
			//128-143       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//144-159       ,   ,   ,242,   ,   ,   ,243,244,245,   ,246,   ,247,   ,252,
			//160-175       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//176-191       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,253,
			//192-207       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//208-223       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//224-239       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//240-255       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
		},
		{
			// koi8-r -> ISO
			/*128-143*/  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			/*144-159*/  144,145,146,147,148,149,150,151,152,153,160,154,155,156,157,158,
			/*160-175*/  159,162,163,241,164,165,166,167,168,169,170,171,172,173,174,175,
			/*176-191*/  240,242,243,161,244,245,246,247,248,249,250,251,252,253,254,255,
			/*192-207*/  238,208,209,230,212,213,228,211,229,216,217,218,219,220,221,222,
			/*208-223*/  223,239,224,225,226,227,214,210,236,235,215,232,237,233,231,234,
			/*224-239*/  206,176,177,198,180,181,196,179,197,184,185,186,187,188,189,190,
			/*240-255*/  191,207,192,193,194,195,182,178,204,203,183,200,205,201,199,202,
			//   Non mapped character
			//128-143    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			//144-159    144,145,146,147,148,149,150,151,152,153,   ,154,155,156,157,158,
			//160-175    159,162,163,   ,164,165,166,167,168,169,170,171,172,173,174,175,
			//176-191    240,242,243,   ,244,245,246,247,248,249,250,251,252,253,254,255,
			//192-207       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//208-223       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//224-239       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//240-255       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
		},
	},
	{
		{
			// 866 -> 1251
			/*128-143*/  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
			/*144-159*/  208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
			/*160-175*/  224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
			/*176-191*/  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			/*192-207*/  144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
			/*208-223*/  160,163,165,166,167,169,171,172,173,174,177,178,179,180,181,182,
			/*224-239*/  240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
			/*240-255*/  168,184,170,186,175,191,161,162,176,187,183,188,185,189,190,160,
			//   Non mapped character
			//128-143    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			//144-159    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
			//160-175    163,164,165,166,167,169,171,172,173,174,177,178,179,180,181,182,
			//176-191       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//192-207       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//208-223       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//224-239       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//240-255       ,   ,   ,   ,   ,   ,   ,   ,   ,187,   ,188,   ,189,190,   ,
		},
		{
			// 866 -> koi8-r
			/*128-143*/  225,226,247,231,228,229,246,250,233,234,235,236,237,238,239,240,
			/*144-159*/  242,243,244,245,230,232,227,254,251,253,255,249,248,252,224,241,
			/*160-175*/  193,194,215,199,196,197,214,218,201,202,203,204,205,206,207,208,
			/*176-191*/  144,145,146,129,135,178,180,167,166,181,161,168,174,173,172,131,
			/*192-207*/  132,137,136,134,128,138,175,176,171,165,187,184,177,160,190,185,
			/*208-223*/  186,182,183,170,169,162,164,189,188,133,130,141,140,142,143,139,
			/*224-239*/  210,211,212,213,198,200,195,222,219,221,223,217,216,220,192,209,
			/*240-255*/  179,163,147,151,152,153,155,157,156,149,158,150,159,191,148,154,
			//   Non mapped character
			//128-143       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//144-159       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//160-175       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//176-191       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//192-207       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//208-223       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//224-239       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//240-255       ,   ,147,151,152,153,155,157,   ,   ,   ,   ,159,191,   ,   ,
		},
		{
			// 866 -> 866 = dummy
			/*128-143*/  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			/*144-159*/  144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
			/*160-175*/  160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
			/*176-191*/  176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
			/*192-207*/  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
			/*208-223*/  208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
			/*224-239*/  224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
			/*240-255*/  240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
		},
		{
			// 866 -> ISO
			/*128-143*/  176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
			/*144-159*/  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
			/*160-175*/  208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
			/*176-191*/  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			/*192-207*/  144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
			/*208-223*/  162,163,165,166,168,169,170,171,172,173,175,240,242,243,245,246,
			/*224-239*/  224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
			/*240-255*/  161,241,164,244,167,247,174,254,248,249,250,251,252,253,255,160,
			//   Non mapped character
			//128-143       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//144-159       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//160-175       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//176-191    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			//192-207    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
			//208-223    162,163,165,166,168,169,170,171,172,173,175,240,242,243,245,246,
			//224-239       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//240-255       ,   ,   ,   ,   ,   ,   ,   ,248,249,250,251,252,253,255,   ,
		},
	},
	{
		{
			// ISO -> 1251
			/*128-143*/  130,132,133,134,135,136,137,139,145,146,147,148,149,150,151,152,
			/*144-159*/  153,155,164,165,166,169,171,172,174,176,177,180,181,182,183,187,
			/*160-175*/  160,168,128,129,170,189,178,175,163,138,140,142,141,173,161,143,
			/*176-191*/  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
			/*192-207*/  208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
			/*208-223*/  224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
			/*224-239*/  240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
			/*240-255*/  185,184,144,131,186,190,179,191,188,154,156,158,157,167,162,159,
			//   Non mapped character
			//128-143    130,132,133,134,135,136,137,139,145,146,147,148,149,150,151,152,
			//144-159    153,155,164,165,166,169,171,172,174,176,177,180,181,182,183,187,
			//160-175       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//176-191       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//192-207       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//208-223       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//224-239       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//240-255       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
		},
		{
			// ISO -> koi8-r
			/*128-143*/  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			/*144-159*/  144,145,146,147,148,149,150,151,152,153,155,156,157,158,159,160,
			/*160-175*/  154,179,161,162,164,165,166,167,168,169,170,171,172,173,174,175,
			/*176-191*/  225,226,247,231,228,229,246,250,233,234,235,236,237,238,239,240,
			/*192-207*/  242,243,244,245,230,232,227,254,251,253,255,249,248,252,224,241,
			/*208-223*/  193,194,215,199,196,197,214,218,201,202,203,204,205,206,207,208,
			/*224-239*/  210,211,212,213,198,200,195,222,219,221,223,217,216,220,192,209,
			/*240-255*/  176,163,177,178,180,181,182,183,184,185,186,187,188,189,190,191,
			//   Non mapped character
			//128-143    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			//144-159    144,145,146,147,148,149,150,151,152,153,155,156,157,158,159,160,
			//160-175       ,   ,161,162,164,165,166,167,168,169,170,171,172,173,174,175,
			//176-191       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//192-207       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//208-223       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//224-239       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//240-255    176,   ,177,178,180,181,182,183,184,185,186,187,188,189,190,191,
		},
		{
			// ISO -> 866
			/*128-143*/  176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
			/*144-159*/  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
			/*160-175*/  255,240,208,209,242,210,211,244,212,213,214,215,216,217,246,218,
			/*176-191*/  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			/*192-207*/  144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
			/*208-223*/  160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
			/*224-239*/  224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
			/*240-255*/  219,241,220,221,243,222,223,245,248,249,250,251,252,253,247,254,
			//   Non mapped character
			//128-143    176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
			//144-159    192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
			//160-175       ,   ,208,209,   ,210,211,   ,212,213,214,215,216,217,   ,218,
			//176-191       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//192-207       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//208-223       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//224-239       ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,   ,
			//240-255    219,   ,220,221,   ,222,223,   ,248,249,250,251,252,253,   ,254,
		},
		{
			// iso -> iso = dummy
			/*128-143*/  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
			/*144-159*/  144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
			/*160-175*/  160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
			/*176-191*/  176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
			/*192-207*/  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
			/*208-223*/  208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
			/*224-239*/  224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
			/*240-255*/  240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
		}
	}
};

// Russian character set conversion
DllExport BYTE PASCAL RussConv(int cin, int cout, BYTE b)
// cin: input character set (IdWindows/IdKOI8/Id866/IdISO)
// cin: output character set (IdWindows/IdKOI8/Id866/IdISO)
{
	if (b<128) {
		return b;
	}
	return cpconv[cin-1][cout-1][b-128];
}

// Russian character set conversion for a character string
DllExport void PASCAL RussConvStr(int cin, int cout, PCHAR Str, int count)
// cin: input character set (IdWindows/IdKOI8/Id866/IdISO)
// cin: output character set (IdWindows/IdKOI8/Id866/IdISO)
{
	int i;

	if (count<=0) {
		return;
	}

	for (i=0; i<=count-1; i++) {
		if ((BYTE)Str[i]>=128) {
			Str[i] = (char)cpconv[cin-1][cout-1][(BYTE)Str[i]-128];
		}
	}
}
