﻿/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005-2018 TeraTerm Project
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

/* TTMACRO.EXE, Tera Term Language interpreter */

#include "teraterm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mbstring.h>
#include <time.h>
#include <errno.h>
#include "ttmdlg.h"
#include "ttmbuff.h"
#include "ttmparse.h"
#include "ttmdde.h"
#include "ttmlib.h"
#include "ttlib.h"
#include "ttmenc.h"
#include "tttypes.h"
#include "ttmonig.h"
#include "ttmacro.h"
#include "ttl.h"
#include "ttl_gui.h"
#include "codeconv.h"

// add 'clipb2var' (2006.9.17 maya)
WORD TTLClipb2Var()
{
	WORD Err;
	TVarId VarId;
	HGLOBAL hText;
	LPSTR clipbText;
	char buf[MaxStrLen];
	int Num = 0;
	char *newbuff;
	static char *cbbuff;
	static int cbbuffsize, cblen;
	HANDLE wide_hText;
	LPWSTR wide_buf;
	int mb_len;
	UINT Cf;

	Err = 0;
	GetStrVar(&VarId, &Err);
	if (Err!=0) return Err;

	// get 2nd arg(optional) if given
	if (CheckParameterGiven()) {
		GetIntVal(&Num, &Err);
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (Num == 0) {
		if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
			Cf = CF_UNICODETEXT;
		}
		else if (IsClipboardFormatAvailable(CF_TEXT)) {
			Cf = CF_TEXT;
		}
		else {
			cblen = 0;
			SetResult(0);
			return Err;
		}
		if (OpenClipboard(NULL) == 0) {
			cblen = 0;
			SetResult(0);
			return Err;
		}

		if (Cf == CF_UNICODETEXT) {
			wide_hText = GetClipboardData(CF_UNICODETEXT);
			if (wide_hText != NULL) {
				wide_buf = (LPWSTR)GlobalLock(wide_hText);
				mb_len = WideCharToMultiByte(CP_ACP, 0, wide_buf, -1, NULL, 0, NULL, NULL);
				hText = GlobalAlloc(GMEM_MOVEABLE, sizeof(CHAR) * mb_len);
				clipbText = (LPSTR)GlobalLock(hText);
				if (hText != NULL) {
					WideCharToMultiByte(CP_ACP, 0, wide_buf, -1, clipbText, mb_len, NULL, NULL);

					cblen = strlen(clipbText);
					if (cbbuffsize <= cblen) {
						if ((newbuff = (char *)realloc(cbbuff, cblen + 1)) == NULL) {
							// realloc failed. fall back to old mode.
							cblen = 0;
							strncpy_s(buf,sizeof(buf),clipbText,_TRUNCATE);
							GlobalUnlock(hText);
							CloseClipboard();
							SetStrVal(VarId, buf);
							SetResult(3);
							return Err;
						}
						cbbuff = newbuff;
						cbbuffsize = cblen + 1;
					}
					strncpy_s(cbbuff, cbbuffsize, clipbText, _TRUNCATE);

					GlobalUnlock(hText);
					GlobalFree(hText);
				}
				GlobalUnlock(wide_hText);
			}
			else {
				cblen = 0;
			}
		}
		else if (Cf == CF_TEXT) {
			hText = GetClipboardData(CF_TEXT);
			if (hText != NULL) {
				clipbText = (LPSTR)GlobalLock(hText);
				cblen = strlen(clipbText);
				if (cbbuffsize <= cblen) {
					if ((newbuff = (char *)realloc(cbbuff, cblen + 1)) == NULL) {
						// realloc failed. fall back to old mode.
						cblen = 0;
						strncpy_s(buf,sizeof(buf),clipbText,_TRUNCATE);
						GlobalUnlock(hText);
						CloseClipboard();
						SetStrVal(VarId, buf);
						SetResult(3);
						return Err;
					}
					cbbuff = newbuff;
					cbbuffsize = cblen + 1;
				}
				strncpy_s(cbbuff, cbbuffsize, clipbText, _TRUNCATE);
				GlobalUnlock(hText);
			}
			else {
				cblen = 0;
			}
		}
		CloseClipboard();
	}

	if (cbbuff != NULL && Num >= 0 && Num * (MaxStrLen - 1) < cblen) {
		if (strncpy_s(buf ,sizeof(buf), cbbuff + Num * (MaxStrLen-1), _TRUNCATE) == STRUNCATE)
			SetResult(2); // Copied string is truncated.
		else {
			SetResult(1);
		}
		SetStrVal(VarId, buf);
	}
	else {
		SetResult(0);
	}

	return Err;
}

// add 'var2clipb' (2006.9.17 maya)
WORD TTLVar2Clipb()
{
	WORD Err;
	TStrVal Str;
	HGLOBAL hText;
	LPSTR clipbText;
	int wide_len;
	HGLOBAL wide_hText;
	LPWSTR wide_buf;

	Err = 0;
	GetStrVal(Str,&Err);
	if (Err!=0) return Err;

	hText = GlobalAlloc(GHND, sizeof(Str));
	clipbText = (LPSTR)GlobalLock(hText);
	strncpy_s(clipbText, sizeof(Str), Str, _TRUNCATE);
	GlobalUnlock(hText);

	wide_len = MultiByteToWideChar(CP_ACP, 0, clipbText, -1, NULL, 0);
	wide_hText = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * wide_len);
	if (wide_hText) {
		wide_buf = (LPWSTR)GlobalLock(wide_hText);
		MultiByteToWideChar(CP_ACP, 0, clipbText, -1, wide_buf, wide_len);
		GlobalUnlock(wide_hText);
	}

	if (OpenClipboard(NULL) == 0) {
		SetResult(0);
	}
	else {
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hText);

		if (wide_buf) {
			SetClipboardData(CF_UNICODETEXT, wide_hText);
		}

		CloseClipboard();
		SetResult(1);
	}

	return Err;
}

// add 'filenamebox' (2007.9.13 maya)
WORD TTLFilenameBox()
{
	TStrVal Str1;
	WORD Err, ValType;
	TVarId VarId;
	OPENFILENAME ofn;
	TCHAR uimsg[MAX_UIMSG];
	BOOL SaveFlag = FALSE;
	TStrVal InitDir = "";
	tc InitDirT;

	Err = 0;
	GetStrVal(Str1,&Err);
	tc Str1T = tc::fromUtf8(Str1);

	if (Err!=0) return Err;

	// get 2nd arg(optional) if given
	if (CheckParameterGiven()) { // dialogtype
		GetIntVal(&SaveFlag, &Err);
		if (Err!=0) return Err;

		// get 3rd arg(optional) if given
		if (CheckParameterGiven()) { // initdir
			GetStrVal(InitDir, &Err);
			if (Err!=0) return Err;
			InitDirT = InitDir;
		}
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetInputStr("");
	if (CheckVar("inputstr", &ValType, &VarId) && (ValType==TypString)) {
		TCHAR filename[MaxStrLen];
		filename[0] = 0;
		memset(&ofn, 0, sizeof(OPENFILENAME));
		//ofn.lStructSize     = sizeof(OPENFILENAME);
		ofn.lStructSize     = OPENFILENAME_SIZE_VERSION_400;		// TODO
		ofn.hwndOwner       = GetHWND();
		ofn.lpstrTitle      = Str1T;
		ofn.lpstrFile       = filename;
		ofn.nMaxFile        = _countof(filename);
		get_lang_msgT("FILEDLG_ALL_FILTER", uimsg, _countof(uimsg), _T("All(*.*)\\0*.*\\0\\0"), UILanguageFile);
		ofn.lpstrFilter     = uimsg;
		ofn.lpstrInitialDir = NULL;
		if (strlen(InitDir) > 0) {
			ofn.lpstrInitialDir = InitDirT;
		}
		BringupWindow(GetHWND());
		BOOL ret;
		if (SaveFlag) {
			ofn.Flags = OFN_OVERWRITEPROMPT;
			ret = GetSaveFileName(&ofn);
		}
		else {
			ret = GetOpenFileName(&ofn);
		}

		const char *filenameU8 = ToU8T(filename);
		char *dest = StrVarPtr(VarId);
		strcpy(dest, filenameU8);
		free((void *)filenameU8);

		SetResult(ret);
	}
	return Err;
}

WORD TTLDirnameBox()
{
	TStrVal Title;
	WORD Err, ValType;
	TVarId VarId;
	TStrVal InitDir = "";
	BOOL ret;

	Err = 0;
	GetStrVal(Title, &Err);
	if (Err != 0) return Err;

	// get 2nd arg(optional) if given
	if (CheckParameterGiven()) { // initdir
		GetStrVal(InitDir, &Err);
		if (Err != 0) return Err;
	}

	if ((Err == 0) && (GetFirstChar() != 0))
		Err = ErrSyntax;
	if (Err != 0) return Err;

	SetInputStr("");
	if (CheckVar("inputstr", &ValType, &VarId) &&
	    (ValType == TypString)) {
		BringupWindow(GetHWND());
		TCHAR buf[MAX_PATH];
		if (doSelectFolderT(GetHWND(), buf, _countof(buf), tc::fromUtf8(InitDir), tc::fromUtf8(Title))) {
			const char *bufU8 = ToU8T(buf);
			SetInputStr((PCHAR)bufU8);
			free((void *)bufU8);
			ret = 1;
		}
		else {
			ret = 0;
		}
		SetResult(ret);
	}
	return Err;
}

int MessageCommand(int BoxId, LPWORD Err)
{
	TStrVal Str1, Str2;
	int sp = 0;
	int ret;
	int i, ary_size;
	int sel = 0;
	TVarId VarId;

	*Err = 0;
	GetStrVal2(Str1, Err, TRUE);
	GetStrVal2(Str2, Err, TRUE);
	if (*Err!=0) return 0;

	if (BoxId != IdListBox) {
		// get 3rd arg(optional) if given
		if (CheckParameterGiven()) {
			GetIntVal(&sp, Err);
		}
		if ((*Err==0) && (GetFirstChar()!=0))
			*Err = ErrSyntax;
		if (*Err!=0) return 0;
	}

	if (sp) {
		RestoreNewLine(Str1);
	}

	if (BoxId==IdMsgBox) {
		ret = OpenMsgDlg(tc::fromUtf8(Str1),tc::fromUtf8(Str2),FALSE);
		// メッセージボックスをキャンセルすると、マクロの終了とする。
		// (2008.8.5 yutaka)
		if (ret == IDCANCEL) {
			TTLStatus = IdTTLEnd;
		}
	} else if (BoxId==IdYesNoBox) {
		ret = OpenMsgDlg(tc::fromUtf8(Str1),tc::fromUtf8(Str2),TRUE);
		// メッセージボックスをキャンセルすると、マクロの終了とする。
		// (2008.8.6 yutaka)
		if (ret == IDCLOSE) {
			TTLStatus = IdTTLEnd;
		}
		return (ret);
	}
	else if (BoxId==IdStatusBox) {
		OpenStatDlg(tc::fromUtf8(Str1),tc::fromUtf8(Str2));

	} else if (BoxId==IdListBox) {
		//  リストボックスの選択肢を取得する。
		GetStrAryVar(&VarId, Err);

		if (CheckParameterGiven()) {
			GetIntVal(&sel, Err);
		}
		if (*Err==0 && GetFirstChar()!=0)
			*Err = ErrSyntax;
		if (*Err!=0) return 0;

		ary_size = GetStrAryVarSize(VarId);
		if (sel < 0 || sel >= ary_size) {
			sel = 0;
		}

		const TCHAR **s = (const TCHAR **)calloc(ary_size + 1, sizeof(TCHAR *));
		if (s == NULL) {
			*Err = ErrFewMemory;
			return -1;
		}
		for (i = 0 ; i < ary_size ; i++) {
			TVarId VarId2;
			VarId2 = GetStrVarFromArray(VarId, i, Err);
			if (*Err!=0) return -1;
			s[i] = ToTcharU8(StrVarPtr(VarId2));
		}
		if (s[0] == NULL) {
			*Err = ErrSyntax;
			return -1;
		}

		// return 
		//   0以上: 選択項目
		//   -1: キャンセル
		//	 -2: close
		ret = OpenListDlg(tc::fromUtf8(Str1), tc::fromUtf8(Str2), s, sel);

		for (i = 0 ; i < ary_size ; i++) {
			free((void *)s[i]);
		}
		free(s);

		// リストボックスの閉じるボタン(&確認ダイアログ)で、マクロの終了とする。
		if (ret == -2) {
			TTLStatus = IdTTLEnd;
		}
		return (ret);

	}
	return 0;
}

// リストボックス
// (2013.3.13 yutaka)
WORD TTLListBox()
{
	WORD Err;
	int ret;

	ret = MessageCommand(IdListBox, &Err);
	SetResult(ret);
	return Err;
}

WORD TTLMessageBox()
{
	WORD Err;

	MessageCommand(IdMsgBox, &Err);
	return Err;
}

/* ttmparse.cから移動 */
extern "C" void DispErr(WORD Err)
{
	const TCHAR *Msg;
	int i;
	int no, start, end;
	char *filename;

	Msg = _T("Unknown error message number.");

	switch (Err) {
		case ErrCloseParent: Msg = _T("\")\" expected."); break;
		case ErrCantCall: Msg = _T("Can't call sub."); break;
		case ErrCantConnect: Msg = _T("Can't link macro."); break;
		case ErrCantOpen: Msg = _T("Can't open file."); break;
		case ErrDivByZero: Msg = _T("Divide by zero."); break;
		case ErrInvalidCtl: Msg = _T("Invalid control."); break;
		case ErrLabelAlreadyDef: Msg = _T("Label already defined."); break;
		case ErrLabelReq: Msg = _T("Label requiered."); break;
		case ErrLinkFirst: Msg = _T("Link macro first. Use 'connect' macro."); break;
		case ErrStackOver: Msg = _T("Stack overflow."); break;
		case ErrSyntax: Msg = _T("Syntax error."); break;
		case ErrTooManyLabels: Msg = _T("Too many labels."); break;
		case ErrTooManyVar: Msg = _T("Too many variables."); break;
		case ErrTypeMismatch: Msg = _T("Type mismatch."); break;
		case ErrVarNotInit: Msg = _T("Variable not initialized."); break;
		case ErrCloseComment: Msg = _T("\"*/\" expected."); break;
		case ErrOutOfRange: Msg = _T("Index out of range."); break;
		case ErrCloseBracket: Msg = _T("\"]\" expected."); break;
		case ErrFewMemory: Msg = _T("Can't allocate memory."); break;
		case ErrNotSupported: Msg = _T("Unknown command."); break;
	}

	no = GetLineNo();
	start = LineParsePtr;
	end = LinePtr;
	if (start == end)
		end = LineLen;

	filename = GetMacroFileName();

	i = OpenErrDlg(Msg, tc::fromUtf8(LineBuff), no, start, end, (tc)filename);
	if (i==IDOK) TTLStatus = IdTTLEnd;
}

WORD TTLGetPassword()
{
	TStrVal Str, Str2, Temp2;
	TCHAR Temp[512];
	WORD Err;
	TVarId VarId;
	int result = 0;  /* failure */

	Err = 0;
	GetStrVal(Str,&Err);
	GetStrVal(Str2,&Err);
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	SetStrVal(VarId,"");
	if (Str[0]==0) return Err;
	if (Str2[0]==0) return Err;

	GetAbsPath(Str,sizeof(Str));

	GetPrivateProfileString(_T("Password"),(tc)Str2,_T(""),
							Temp,_countof(Temp), tc::fromUtf8(Str));
	if (Temp[0]==0) // password not exist
	{
#if defined(UNICODE)
		TCHAR input_string[MaxStrLen];
		OpenInpDlg(input_string, tc::fromUtf8(Str2), _T("Enter password"), _T(""), TRUE);
		WideCharToMultiByte(CP_UTF8, 0,
							input_string, -1,
							Temp2, _countof(Temp2),
							NULL, NULL);
#else
		OpenInpDlg(Temp2, Str2, _T("Enter password"), _T(""), TRUE);
#endif
		if (Temp2[0]!=0) {
			char TempA[512];
			Encrypt(Temp2, TempA);
			if (WritePrivateProfileString(_T("Password"), (tc)Str2, (tc)TempA, tc::fromUtf8(Str)) != 0) {
				result = 1;  /* success */
			}
		}
	}
	else {// password exist
		u8 TempU8 = Temp;
		Decrypt((PCHAR)(const char *)TempU8,Temp2);
		result = 1;  /* success */
	}

	SetStrVal(VarId,Temp2);

	SetResult(result);  // 成功可否を設定する。
	return Err;
}

WORD TTLInputBox(BOOL Paswd)
{
	TStrVal Str1, Str2, Str3;
	WORD Err, ValType, P;
	TVarId VarId;
	int sp = 0;

	Err = 0;
	GetStrVal(Str1,&Err);
	GetStrVal(Str2,&Err);
	if (Err!=0) return Err;

	if (!Paswd && CheckParameterGiven()) {
		// get 3rd arg(optional)
		P = LinePtr;
		GetStrVal(Str3,&Err);
		if (Err == ErrTypeMismatch) {
			strncpy_s(Str3,sizeof(Str3),"",_TRUNCATE);
			LinePtr = P;
			Err = 0;
		}
	}
	else {
		strncpy_s(Str3,sizeof(Str3),"",_TRUNCATE);
	}

	// get 4th(3rd) arg(optional) if given
	if (CheckParameterGiven()) {
		GetIntVal(&sp, &Err);
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (sp) {
		RestoreNewLine(Str1);
	}

	SetInputStr("");
	if (CheckVar("inputstr",&ValType,&VarId) && (ValType==TypString)) {
#if defined(UNICODE)
		TCHAR input_string[MaxStrLen];
		OpenInpDlg(input_string,tc::fromUtf8(Str1),tc::fromUtf8(Str2),tc::fromUtf8(Str3),Paswd);
		char *output = StrVarPtr(VarId);
		WideCharToMultiByte(CP_UTF8, 0,
							input_string, -1,
							output, MaxStrLen,
							NULL, NULL);
#else
		OpenInpDlg(StrVarPtr(VarId),Str1,Str2,Str3,Paswd);
#endif
	}
	return Err;
}