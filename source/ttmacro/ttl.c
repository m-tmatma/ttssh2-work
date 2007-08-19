/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, Tera Term Language interpreter */

#include "teraterm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ttmdlg.h"
#include "ttmbuff.h"
#include "ttmparse.h"
#include "ttmdde.h"
#include "ttmlib.h"
#include "ttlib.h"
#include "ttmenc.h"

// Oniguruma: Regular expression library
#define ONIG_EXTERN extern
#include "oniguruma.h"
#undef ONIG_EXTERN

// for _findXXXX() functions
#include <io.h>

// for _ismbblead
#include <mbctype.h>

#include "ttl.h"

#ifdef TERATERM32
  #define TTERMCOMMAND "TTERMPRO /D="
#else
  #include <sys/stat.h>
  #include <sys/utime.h>
  #include <i86.h>
  #define TTERMCOMMAND "TERATERM /D="
#endif

// for 'ExecCmnd' command
static BOOL ParseAgain;
static int IfNest;
static int ElseFlag;
static int EndIfFlag;
// Window handle of the main window
static HWND HMainWin;
// Timeout
static DWORD TimeLimit;
static DWORD TimeStart;
// for 'WaitEvent' command
static int WakeupCondition;

// exit code of TTMACRO
int ExitCode = 0;

// for "FindXXXX" commands
#define NumDirHandle 8
static long DirHandle[NumDirHandle];
/* for "FileMarkPtr" and "FileSeekBack" commands */
#define NumFHandle 16
static int FHandle[NumFHandle];
static long FPointer[NumFHandle];

// forward declaration
int ExecCmnd();

BOOL InitTTL(HWND HWin)
{
	int i;
	TStrVal Dir;

	HMainWin = HWin;

	if (! InitVar()) return FALSE;
	LockVar();

	// System variables
	NewIntVar("result",0);
	NewIntVar("timeout",0);
	NewStrVar("inputstr","");
	NewStrVar("matchstr","");   // for 'waitregex' command (2005.10.7 yutaka)
	NewStrVar("groupmatchstr1","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr2","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr3","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr4","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr5","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr6","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr7","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr8","");   // for 'waitregex' command (2005.10.15 yutaka)
	NewStrVar("groupmatchstr9","");   // for 'waitregex' command (2005.10.15 yutaka)

	NewStrVar("param2",Param2);
	NewStrVar("param3",Param3);

	ParseAgain = FALSE;
	IfNest = 0;
	ElseFlag = 0;
	EndIfFlag = 0;

	for (i=0; i<NumDirHandle; i++)
		DirHandle[i] = -1L;
	for (i=0; i<NumFHandle; i++)
		FHandle[i] = -1;

	if (! InitBuff(FileName))
	{
		TTLStatus = IdTTLEnd;
		return FALSE;
	}

	UnlockVar();

	ExtractDirName(FileName,Dir);
	TTMSetDir(Dir);

	if (SleepFlag)
	{ // synchronization for startup macro
		// sleep until Tera Term is ready
		WakeupCondition = IdWakeupInit;
		// WakeupCondition = IdWakeupConnect | IdWakeupDisconn | IdWakeupUnlink;
		TTLStatus = IdTTLSleep;
	}
	else
		TTLStatus = IdTTLRun;

	return TRUE;
}

void EndTTL()
{
	int i;

	CloseStatDlg();

	for (i=0; i<NumDirHandle; i++)
	{
		if (DirHandle[i]!=-1L)
			_findclose(DirHandle[i]);
		DirHandle[i] = -1L;
	}

	UnlockVar();
	if (TTLStatus==IdTTLWait)
		KillTimer(HMainWin,IdTimeOutTimer);
	CloseBuff(0);
	EndVar();
}

long int CalcTime()
{
	time_t time1;
	struct tm *ptm;

	time1 = time(NULL);
	ptm = localtime(&time1);
	return ((long int)ptm->tm_hour * 3600 +
	        (long int)ptm->tm_min * 60 +
	        (long int)ptm->tm_sec
	       );
}

//////////////// Beginning of TTL commands //////////////
WORD TTLCommCmd(char Cmd, int Wait)
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	else if (! Linked)
		return ErrLinkFirst;
	else
		return SendCmnd(Cmd,Wait);
}

WORD TTLCommCmdFile(char Cmd, int Wait)
{
	TStrVal Str;
	WORD Err;

	Err = 0;
	GetStrVal(Str,&Err);

	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err==0)
	{
		SetFile(Str);
		Err = SendCmnd(Cmd,Wait);
	}
	return Err;
}

WORD TTLCommCmdBin(char Cmd, int Wait)
{
	int Val;
	WORD Err;

	Err = 0;
	GetIntVal(&Val,&Err);

	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err==0)
	{
		SetBinary(Val);
		Err = SendCmnd(Cmd,Wait);
	}
	return Err;
}

WORD TTLCommCmdInt(char Cmd, int Wait)
{
	int Val;
	char Str[21];
	WORD Err;

	Err = 0;
	GetIntVal(&Val,&Err);

	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err==0)
	{
		_snprintf_s(Str,sizeof(Str),_TRUNCATE,"%d",Val);
		SetFile(Str);
		Err = SendCmnd(Cmd,Wait);
	}
	return Err;
}

WORD TTLBeep()
{
	if (GetFirstChar()==0)
	{
		MessageBeep(0);
		return 0;
	}
	else
		return ErrSyntax;
}

WORD TTLBreak() {
	if (GetFirstChar()!=0)
		return ErrSyntax;

	return BreakLoop();
}

WORD TTLCall()
{
	TName LabName;
	WORD Err, VarType, VarId;

	if (GetLabelName(LabName) && (GetFirstChar()==0))
	{
		if (CheckVar(LabName,&VarType,&VarId) && (VarType==TypLabel))
			Err = CallToLabel(VarId);
		else
			Err = ErrLabelReq;
	}
	else
	Err = ErrSyntax;

	return Err;
}

// add 'clipb2var' (2006.9.17 maya)
WORD TTLClipb2Var()
{
	WORD VarId, Err;
	HANDLE hText;
	PTSTR clipbText;
	char buf[MaxStrLen];

	Err = 0;
	GetStrVar(&VarId,&Err);
	if (Err!=0) return Err;

	if (OpenClipboard(NULL) == 0) {
		SetResult(0);
		return Err;
	}
	hText = GetClipboardData(CF_TEXT);
	if (hText != NULL) {
		clipbText = GlobalLock(hText);
		strncpy_s(buf,sizeof(buf),clipbText,_TRUNCATE);
		GlobalUnlock(hText);
		SetStrVal(VarId,buf);
		SetResult(1);
	}
	else {
		SetResult(0);
	}
	CloseClipboard();

	return Err;
}

// add 'var2clipb' (2006.9.17 maya)
WORD TTLVar2Clipb()
{
	WORD Err;
	TStrVal Str;
	HGLOBAL hText;
	PTSTR clipbText;

	Err = 0;
	GetStrVal(Str,&Err);
	if (Err!=0) return Err;

	hText = GlobalAlloc(GHND, sizeof(Str));
	clipbText = GlobalLock(hText);
	strncpy_s(clipbText, sizeof(Str), Str, _TRUNCATE);
	GlobalUnlock(hText);

	if (OpenClipboard(NULL) == 0) {
		SetResult(0);
	}
	else {
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hText);
		CloseClipboard();
		SetResult(1);
	}

	return Err;
}

WORD TTLCloseSBox()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	CloseStatDlg();
	return 0;
}

WORD TTLCloseTT()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (! Linked)
		return ErrLinkFirst;
	else {
		// Close Tera Term
		SendCmnd(CmdCloseWin,IdTTLWaitCmndEnd);
		EndDDE();
		return 0;
	}
}

WORD TTLCode2Str()
{
	WORD VarId, Err;
	int Num, Len, c, i;
	BYTE d;
	TStrVal Str;

	Err = 0;
	GetStrVar(&VarId,&Err);

	GetIntVal(&Num,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Len = sizeof(Num);
	i = 0;
	for (c=0; c<=Len-1; c++)
	{
		d = (Num >> ((Len-1-c)*8)) & 0xff;
		if ((i>0) || (d!=0))
		{
			Str[i] = d;
			i++;
		}
	}
	Str[i] = 0;
	SetStrVal(VarId,Str);
	return Err;
}

WORD TTLConnect()
{
	TStrVal Cmnd, Str;
	WORD Err;
	WORD w;

	Str[0] = 0;

	Err = 0;
	GetStrVal(Str,&Err);

	if (Err!=0) return Err;

	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (Linked)
	{
		if (ComReady!=0)
		{
			SetResult(2);
			return Err;
		}
		// new connection
		SetFile(Str);
		SendCmnd(CmdConnect,0);

		WakeupCondition = IdWakeupInit;
		TTLStatus = IdTTLSleep;
		return Err;
	}

	SetResult(0);
	// link to Tera Term
	if (strlen(TopicName)==0)
	{
		strncpy_s(Cmnd, sizeof(Cmnd),TTERMCOMMAND, _TRUNCATE);
#ifdef TERATERM32
		w = HIWORD(HMainWin);
		Word2HexStr(w,TopicName);
		w = LOWORD(HMainWin);
		Word2HexStr(w,&(TopicName[4]));
#else
		w = HMainWin;
		Word2HexStr(w,TopicName);
#endif
		strncat_s(Cmnd,sizeof(Cmnd),TopicName,_TRUNCATE);
		strncat_s(Cmnd,sizeof(Cmnd)," ",_TRUNCATE);
		strncat_s(Cmnd,sizeof(Cmnd),Str,_TRUNCATE);
		if (WinExec(Cmnd,SW_SHOW)<32)
			return ErrCantConnect;
		TTLStatus = IdTTLInitDDE;
	}
	return Err;
}

WORD TTLCircularLeftShift()
{
	WORD VarId, Err;
	int x, n;
	unsigned int u_x;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetIntVal(&x,&Err);
	GetIntVal(&n,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	u_x = x;
	SetIntVal(VarId, (x << n) | (u_x >> (32-n)));
	return Err;
}

WORD TTLCircularRightShift()
{
	WORD VarId, Err;
	int x, n;
	unsigned int u_x;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetIntVal(&x,&Err);
	GetIntVal(&n,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	u_x = x;
	SetIntVal(VarId, (u_x >> n) | (x << (32-n)));
	return Err;
}

WORD TTLDelPassword()
{
	TStrVal Str, Str2;
	WORD Err;

	Err = 0;
	GetStrVal(Str,&Err);
	GetStrVal(Str2,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	if (Str[0]==0) return Err;

	GetAbsPath(Str,sizeof(Str));
	if (! DoesFileExist(Str)) return Err;
	if (Str2[0]==0) // delete all password
		WritePrivateProfileString("Password",NULL,NULL,Str);
	else            // delete password specified by Str2
		WritePrivateProfileString("Password",Str2,NULL,Str);
	return Err;
}

WORD TTLElse()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (IfNest<1)
		return ErrInvalidCtl;

	// Skip until 'EndIf'
	IfNest--;
	EndIfFlag = 1;
	return 0;
}

int CheckElseIf(LPWORD Err)
{
	int Val;
	WORD WId;

	*Err = 0;
	GetIntVal(&Val,Err);
	if (*Err!=0) return 0;
	if (! GetReservedWord(&WId) ||
	    (WId!=RsvThen) ||
	    (GetFirstChar()!=0))
		*Err = ErrSyntax;
	return Val;
}

WORD TTLElseIf()
{
	WORD Err;
	int Val;

	Val = CheckElseIf(&Err);
	if (Err!=0) return Err;

	if (IfNest<1)
		return ErrInvalidCtl;

	// Skip until 'EndIf'
	IfNest--;
	EndIfFlag = 1;
	return Err;
}

WORD TTLEnd()
{
	if (GetFirstChar()==0)
	{
		TTLStatus = IdTTLEnd;
		return 0;
	}
	else
		return ErrSyntax;
}

WORD TTLEndIf()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (IfNest<1)
		return ErrInvalidCtl;

	IfNest--;
	return 0;
}

WORD TTLEndWhile()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	return BackToWhile();
}

WORD TTLExec()
{
	TStrVal Str;
	WORD Err;

	Err = 0;
	GetStrVal(Str,&Err);

	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	WinExec(Str,SW_SHOW);
	return Err;
}

WORD TTLExecCmnd()
{
	WORD Err;
	TStrVal NextLine;
	BYTE b;

	Err = 0;
	GetStrVal(NextLine,&Err);
	if (Err!=0) return Err;
	if (GetFirstChar()!=0)
		return ErrSyntax;

	strncpy_s(LineBuff, sizeof(LineBuff),NextLine, _TRUNCATE);
	LineLen = strlen(LineBuff);
	LinePtr = 0;
	b = GetFirstChar();
	LinePtr--;
	ParseAgain = (b!=0) && (b!=':') && (b!=';');
	return Err;
}

WORD TTLExit()
{
	if (GetFirstChar()==0)
	{
		if (! ExitBuffer())
			TTLStatus = IdTTLEnd;
		return 0;
	}
	else
	return ErrSyntax;
}

WORD TTLFileClose()
{
	WORD Err;
	int FH, i;

	Err = 0;
	GetIntVal(&FH,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	_lclose(FH);
	i = 0;
	while ((i<NumFHandle) && (FH!=FHandle[i])) i++;
	if (i<NumFHandle) FHandle[i] = -1;
	return Err;
}

WORD TTLFileConcat()
{
	WORD Err;
	int FH1, FH2, c;
	TStrVal FName1, FName2;
	BYTE buf[1024];

	Err = 0;
	GetStrVal(FName1,&Err);
	GetStrVal(FName2,&Err);
	if ((Err==0) &&
	    ((strlen(FName1)==0) ||
	     (strlen(FName2)==0) ||
	     (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName1,sizeof(FName1))) {
		SetResult(-1);
		return Err;
	}
	if (!GetAbsPath(FName2,sizeof(FName2))) {
		SetResult(-2);
		return Err;
	}
	if (_stricmp(FName1,FName2)==0) {
		SetResult(2);
		return Err;
	}

	FH1 = _lopen(FName1,OF_WRITE);
	if (FH1<0)
		FH1 = _lcreat(FName1,0);
	if (FH1<0) {
		SetResult(3);
		return Err;
	}
	_llseek(FH1,0,2);

	FH2 = _lopen(FName2,OF_READ);
	if (FH2!=-1)
	{
		do {
			c = _lread(FH2,&(buf[0]),sizeof(buf));
			if (c>0)
				_lwrite(FH1,&(buf[0]),c);
		} while (c >= sizeof(buf));
		_lclose(FH2);
	}
	_lclose(FH1);

	SetResult(0);
	return Err;
}

WORD TTLFileCopy()
{
	WORD Err;
	TStrVal FName1, FName2;
#ifndef TERATERM32
	int FH1, FH2, c;
	BYTE buf[1024];
	struct _stat st;
	struct _utimbuf ut;
#endif

	Err = 0;
	GetStrVal(FName1,&Err);
	GetStrVal(FName2,&Err);
	if ((Err==0) &&
	    ((strlen(FName1)==0) ||
	     (strlen(FName2)==0) ||
	     (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName1,sizeof(FName1))) {
		SetResult(-1);
		return Err;
	}
	if (!GetAbsPath(FName2,sizeof(FName2))) {
		SetResult(-2);
		return Err;
	}
	if (_stricmp(FName1,FName2)==0) return Err;

#ifdef TERATERM32
	CopyFile(FName1,FName2,FALSE);
#else
	FH1 = _lopen(FName1,OF_READ);
	if (FH1<0) return Err;
	FH2 = _lcreat(FName2,0);
	if (FH2!=-1)
	{
		do {
			c = _lread(FH1,&(buf[0]),sizeof(buf));
			if (c>0)
				_lwrite(FH2,&(buf[0]),c);
		} while (c >= sizeof(buf));
		_lclose(FH2);
	}
	_lclose(FH1);
	_stat(FName1,&st);
	ut.actime = st.st_atime;
	ut.modtime = st.st_mtime;
	_utime(FName2,&ut);
#endif
	SetResult(0);
	return Err;
}

WORD TTLFileCreate()
{
	WORD Err, VarId;
	int FH;
	TStrVal FName;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName,sizeof(FName))) {
		SetIntVal(VarId,-1);
		SetResult(-1);
		return Err;
	}
	FH = _lcreat(FName,0);
	if (FH<0) {
		FH = -1;
		SetResult(2);
	  }
	  else {
		SetResult(0);
	}
	SetIntVal(VarId,FH);
	return Err;
}

WORD TTLFileDelete()
{
	WORD Err;
	TStrVal FName;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}

	if (!GetAbsPath(FName,sizeof(FName))) {
		SetResult(-1);
		return Err;
	}
	remove(FName);

	SetResult(0);
	return Err;
}

WORD TTLFileMarkPtr()
{
	WORD Err;
	int FH, i;

	Err = 0;
	GetIntVal(&FH,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	i = 0;
	while ((i<NumFHandle) && (FH!=FHandle[i])) i++;
	if (i>=NumFHandle)
	{
		i = 0;
		while ((i<NumFHandle) && (FHandle[i]!=-1)) i++;
		if (i<NumFHandle) FHandle[i] = FH;
	}
	if (i<NumFHandle)
	{
		FPointer[i] = _llseek(FH,0,1); /* mark current pos */
		if (FPointer[i]<0) FPointer[i] = 0;
	}
	return Err;
}

WORD TTLFileOpen()
{
	WORD Err, VarId;
	int Append, FH;
	TStrVal FName;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetStrVal(FName,&Err);
	GetIntVal(&Append,&Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (!GetAbsPath(FName,sizeof(FName))) {
		SetIntVal(VarId,-1);
		return Err;
	}
	FH = _lopen(FName,OF_READWRITE);
	if (FH<0)
		FH = _lcreat(FName,0);
	if (FH<0) FH = -1;
	SetIntVal(VarId,FH);
	if (FH<0) return Err;
	if (Append!=0) _llseek(FH,0,2);  
	return Err;
}

WORD TTLFileReadln()
{
	WORD Err, VarId;
	int FH, i, c;
	TStrVal Str;
	BOOL EndFile, EndLine;
	BYTE b;

	Err = 0;
	GetIntVal(&FH,&Err);
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	i = 0;
	EndLine = FALSE;
	EndFile = TRUE;
	do {
		c = _lread(FH,&b,1);
		if (c>0) EndFile = FALSE;
		if (c==1) {
			switch (b) {
				case 0x0d:
					c = _lread(FH,&b,1);
					if ((c==1) && (b!=0x0a))
						_llseek(FH,-1,1);
					EndLine = TRUE;
					break;
				case 0x0a: EndLine = TRUE; break;
				default:
					if (i<MaxStrLen-1)
					{
						Str[i] = b;
						i++;
					}
			}
		}
	} while ((c>=1) && (! EndLine));

	if (EndFile)
		SetResult(1);
	else
		SetResult(0);

	Str[i] = 0;
	SetStrVal(VarId,Str);
	return Err;
}


// Format: fileread <file handle> <read byte> <strvar>
// 指定したバイト数だけファイルから読み込む。
// ただし、<read byte>は 1〜255 まで。
// (2006.11.1 yutaka)
WORD TTLFileRead()
{
	WORD Err, VarId;
	int FH, i, c;
	int ReadByte;   // 読み込むバイト数
	TStrVal Str;
	BOOL EndFile, EndLine;
	BYTE b;

	Err = 0;
	GetIntVal(&FH,&Err);
	GetIntVal(&ReadByte,&Err);
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (ReadByte < 1 || ReadByte > MaxStrLen-1))  // 範囲チェック
		Err = ErrSyntax;
	if (Err!=0) return Err;

	EndLine = FALSE;
	EndFile = FALSE;
	for (i = 0 ; i < ReadByte ; i++) {
		c = _lread(FH,&b,1);
		if (c <= 0) {  // EOF
			EndFile = TRUE;
			break;
		}
		if (i<MaxStrLen-1)
		{
			Str[i] = b;
		}
	} 

	if (EndFile)
		SetResult(1);
	else
		SetResult(0);

	Str[i] = 0;
	SetStrVal(VarId,Str);
	return Err;
}


WORD TTLFileRename()
{
	WORD Err;
	TStrVal FName1, FName2;

	Err = 0;
	GetStrVal(FName1,&Err);
	GetStrVal(FName2,&Err);
	if ((Err==0) &&
	    ((strlen(FName1)==0) ||
	     (strlen(FName2)==0) ||
	     (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) {
		SetResult(1);
		return Err;
	}
	if (_stricmp(FName1,FName2)==0) {
		SetResult(2);
		return Err;
	}

	if (!GetAbsPath(FName1,sizeof(FName1))) {
		SetResult(-1);
		return Err;
	}
	if (!GetAbsPath(FName2,sizeof(FName2))) {
		SetResult(-2);
		return Err;
	}
	rename(FName1,FName2);

	SetResult(0);
	return Err;
}

WORD TTLFileSearch()
{
	WORD Err;
	TStrVal FName;

	Err = 0;
	GetStrVal(FName,&Err);
	if ((Err==0) &&
	    ((strlen(FName)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	GetAbsPath(FName,sizeof(FName));
	if (DoesFileExist(FName))
		SetResult(1);
	else
		SetResult(0);
	return Err;
}

WORD TTLFileSeek()
{
	WORD Err;
	int FH, i, j;

	Err = 0;
	GetIntVal(&FH,&Err);
	GetIntVal(&i,&Err);
	GetIntVal(&j,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	_llseek(FH,i,j);
	return Err;
}

WORD TTLFileSeekBack()
{
	WORD Err;
	int FH, i;

	Err = 0;
	GetIntVal(&FH,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	i = 0;
	while ((i<NumFHandle) && (FH!=FHandle[i])) i++;
	/* move back to the marked pos */
	if (i<NumFHandle)
		_llseek(FH,FPointer[i],0);
	return Err;
}

WORD TTLFileStrSeek()
{
	WORD Err;
	int FH, Len, i, c;
	TStrVal Str;
	BYTE b;
	long int pos;

	Err = 0;
	GetIntVal(&FH,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	pos = _llseek(FH,0,1);
	if (pos==-1) return Err;

	Len = strlen(Str);
	i = 0;
	do {
		c = _lread(FH,&b,1);
		if (c==1)
		{
			if (b==(BYTE)Str[i])
				i++;
			else if (i>0) {
				i = 0;
				if (b==(BYTE)Str[0])
					i = 1;
			}
		}
	} while ((c>=1) && (i!=Len));
	if (i==Len)
		SetResult(1);
	else {
		SetResult(0);
		_llseek(FH,pos,0);
	}
	return Err;
}

WORD TTLFileStrSeek2()
{
	WORD Err;
	int FH, Len, i, c;
	TStrVal Str;
	BYTE b;
	long int pos, pos2;
	BOOL Last;

	Err = 0;
	GetIntVal(&FH,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	pos = _llseek(FH,0,1);
	if (pos<=0) return Err;

	Len = strlen(Str);
	i = 0;
	pos2 = pos;
	do {
		Last = (pos2<=0);
		c = _lread(FH,&b,1);
		pos2 = _llseek(FH,-2,1);
		if (c==1)
		{
			if (b==(BYTE)Str[Len-1-i])
				i++;
			else if (i>0) {
				i = 0;
				if (b==(BYTE)Str[Len-1])
					i = 1;
			}
		}
	} while (!Last && (i!=Len));
	if (i==Len)
		SetResult(1);
	else {
		SetResult(0);
		_llseek(FH,pos,0);
	}
	return Err;
}

WORD TTLFileWrite()
{
	WORD Err;
	int FH;
	TStrVal Str;

	Err = 0;
	GetIntVal(&FH,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	_lwrite(FH,Str,strlen(Str));
	return Err;
}

WORD TTLFileWriteLn()
{
	WORD Err;
	int FH;
	TStrVal Str;

	Err = 0;
	GetIntVal(&FH,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	_lwrite(FH,Str,strlen(Str));
	_lwrite(FH,"\015\012",2);
	return Err;
}

WORD TTLFindClose()
{
	WORD Err;
	int DH;

	Err = 0;
	GetIntVal(&DH,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if ((DH>=0) && (DH<NumDirHandle) &&
	    (DirHandle[DH]!=-1L))
	{
		_findclose(DirHandle[DH]);
		DirHandle[DH] = -1L;
	}
	return Err;
}

WORD TTLFindFirst()
{
	WORD DH, Name, Err;
	TStrVal Dir;
	int i;
	struct _finddata_t data;

	Err = 0;
	GetIntVar(&DH,&Err);
	GetStrVal(Dir,&Err);
	GetStrVar(&Name,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (Dir[0]==0) strncpy_s(Dir, sizeof(Dir),"*.*", _TRUNCATE);
	GetAbsPath(Dir,sizeof(Dir));
	i = 0;
	while ((i<NumDirHandle) &&
	       (DirHandle[i]!=-1L))
		i++;
	if (i<NumDirHandle)
	{
		DirHandle[i] = _findfirst(Dir,&data);
		if (DirHandle[i]!=-1L)
			SetStrVal(Name,data.name);
		else
			i = -1;
	}
	else
		i = -1;

	SetIntVal(DH,i);
	if (i<0)
	{
		SetResult(0);
		SetStrVal(Name,"");
	}
	else
		SetResult(1);
	return Err;
}

WORD TTLFindNext()
{
	WORD Name, Err;
	int DH;
	struct _finddata_t data;

	Err = 0;
	GetIntVal(&DH,&Err);
	GetStrVar(&Name,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if ((DH>=0) && (DH<NumDirHandle) &&
	    (DirHandle[DH]!=-1L) &&
	    (_findnext(DirHandle[DH],&data)==0))
	{
		SetStrVal(Name,data.name);
		SetResult(1);
	}
	else {
		SetStrVal(Name,"");
		SetResult(0);
	}
	return Err;
}

WORD TTLFlushRecv()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	FlushRecv();
	return 0;
}

WORD TTLFor()
{
	WORD Err, VarId;
	int ValStart, ValEnd, i;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetIntVal(&ValStart,&Err);
	GetIntVal(&ValEnd,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (! CheckNext())
	{ // first time
		Err = SetForLoop();
		if (Err==0)
		{
			SetIntVal(VarId,ValStart);
			i = CopyIntVal(VarId);
			if (i==ValEnd)
				LastForLoop();
		}
	}
	else { // return from 'next'
		i = CopyIntVal(VarId);
		if (i<ValEnd)
			i++;
		else if (i>ValEnd)
			i--;
		SetIntVal(VarId,i);
		if (i==ValEnd)
			LastForLoop();
	}
	return Err;
}

WORD TTLGetDate()
{
	WORD VarId, Err;
	TStrVal Str2;
	time_t time1;
	struct tm *ptm;

	Err = 0;
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	// get current date & time
	time1 = time(NULL);
	ptm = localtime(&time1);
	_snprintf_s(Str2,sizeof(Str2),_TRUNCATE,"%4.4d-%2.2d-%2.2d",
	            ptm->tm_year+1900,
	            ptm->tm_mon+1,
	            ptm->tm_mday);
	SetStrVal(VarId,Str2);
	return Err;
}

WORD TTLGetDir()
{
	WORD VarId, Err;
	TStrVal Str;

	Err = 0;
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	TTMGetDir(Str);
	SetStrVal(VarId,Str);
	return Err;
}

WORD TTLGetEnv()
{
	WORD VarId, Err;
	TStrVal Str;
	PCHAR Str2;

	Err = 0;
	GetStrVal(Str,&Err);
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Str2 = getenv(Str);

	if (Str2!=NULL)
		SetStrVal(VarId,Str2);
	else
		SetStrVal(VarId,"");
	return Err;
}

WORD TTLGetPassword()
{
	TStrVal Str, Str2, Temp2;
	char Temp[512];
	WORD VarId, Err;

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
	GetPrivateProfileString("Password",Str2,"",
	                        Temp,sizeof(Temp),Str);
	if (Temp[0]==0) // password not exist
	{
		OpenInpDlg(Temp2, Str2, "Enter password", "", TRUE);
		if (Temp2[0]!=0) {
			Encrypt(Temp2,Temp);
			WritePrivateProfileString("Password",Str2,Temp,Str);
		}
	}
	else // password exist
		Decrypt(Temp,Temp2);

	SetStrVal(VarId,Temp2);
	return Err;
}

WORD TTLGetTime()
{
	WORD VarId, Err;
	TStrVal Str2;
	time_t time1;
	struct tm *ptm;

	Err = 0;
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	// get current date & time
	time1 = time(NULL);
	ptm = localtime(&time1);

	_snprintf_s(Str2,sizeof(Str2),_TRUNCATE,"%2.2d:%2.2d:%2.2d",
	            ptm->tm_hour,
	            ptm->tm_min,
	            ptm->tm_sec);

	SetStrVal(VarId,Str2);
	return Err;
}

WORD TTLGetTitle()
{
	WORD VarId, Err;
	TName Str;

	Err = 0;
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;

	Err = GetTTParam(CmdGetTitle,Str,sizeof(Str));
	if (Err==0)
		SetStrVal(VarId,Str);
	return Err;
}

WORD TTLGoto()
{
	TName LabName;
	WORD Err, VarType, VarId;

	if (GetLabelName(LabName) && (GetFirstChar()==0))
	{
		if (CheckVar(LabName,&VarType,&VarId) && (VarType==TypLabel))
		{
			JumpToLabel(VarId);
			Err = 0;
		}
		else
			Err = ErrLabelReq;
	}
	else
		Err = ErrSyntax;

	return Err;
}

// add 'ifdefined' (2006.9.23 maya)
WORD TTLIfDefined()
{
	WORD VarType, Err;
	int Val;

	GetVarType(&VarType,&Val,&Err);

	SetResult(VarType);

	return Err;
}

BOOL CheckThen(LPWORD Err)
{
	BYTE b;
	TName Temp;

	do {

		do {
			b = GetFirstChar();
			if (b==0) return FALSE;
		} while (((b<'A') || (b>'Z')) &&
		         (b!='_') &&
		         ((b<'a') || (b>'z')));
		LinePtr--;
		if (! GetIdentifier(Temp)) return FALSE;

	} while (_stricmp(Temp,"then")!=0);

	if (GetFirstChar()!=0)
		*Err = ErrSyntax;
	return TRUE;
}

WORD TTLIf()
{
	WORD Err, ValType, Tmp, WId;
	int Val;

	if (! GetOrResult(&ValType,&Val,&Err))
		return ErrSyntax;

	if (Err!=0) return Err;

	if (ValType!=TypInteger)
		return ErrTypeMismatch;

	Tmp = LinePtr;
	if (GetReservedWord(&WId) &&
	    (WId==RsvThen))
	{  // If then  ... EndIf structure
		if (GetFirstChar()!=0)
			return ErrSyntax;
		IfNest++;
		if (Val==0)
			ElseFlag = 1; // Skip until 'Else' or 'EndIf'
	}
	else { // single line lf command
		if (Val==0)
			return 0;
		LinePtr = Tmp;
		return ExecCmnd();
	}
	return Err;
}

WORD TTLInclude()
{
	WORD Err;
	TStrVal Str;

	Err = 0;
	GetStrVal(Str,&Err);
	if (!GetAbsPath(Str,sizeof(Str))) {
		Err = ErrCantOpen;
		return Err;
	}
	if (! BuffInclude(Str)) {
		Err = ErrCantOpen;
		return Err;
	}

	return Err;
}

WORD TTLInputBox(BOOL Paswd)
{
	TStrVal Str1, Str2, Str3;
	WORD Err, ValType, VarId;

	Err = 0;
	GetStrVal(Str1,&Err);
	GetStrVal(Str2,&Err);
	if (Err!=0) return Err;

	// get 3rd arg(optional)
	GetStrVal(Str3,&Err);
	if (Err == ErrSyntax) { // missing 3rd arg is not an error
		strncpy_s(Str3,sizeof(Str3),"",_TRUNCATE);
		Err = 0;
	}

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetInputStr("");
	if (CheckVar("inputstr",&ValType,&VarId) &&
	    (ValType==TypString))
		OpenInpDlg(StrVarPtr(VarId),Str1,Str2,Str3,Paswd);
	return Err;
}

WORD TTLInt2Str()
{
	WORD VarId, Err;
	int Num;
	TStrVal Str2;

	Err = 0;
	GetStrVar(&VarId,&Err);

	GetIntVal(&Num,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	_snprintf_s(Str2,sizeof(Str2),_TRUNCATE,"%d",Num);

	SetStrVal(VarId,Str2);
	return Err;
}

WORD TTLLogOpen()
{
	TStrVal Str;
	WORD Err;
	int BinFlag, AppendFlag;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&BinFlag,&Err);
	GetIntVal(&AppendFlag,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	SetFile(Str);
	SetBinary(BinFlag);
	SetAppend(AppendFlag);
	return SendCmnd(CmdLogOpen,0);
}

WORD TTLMakePath()
{
	WORD VarId, Err;
	TStrVal Dir, Name;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Dir,&Err);
	GetStrVal(Name,&Err);
	if ((Err==0) &&
	    (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	AppendSlash(Dir,sizeof(Dir));
	strncat_s(Dir,sizeof(Dir),Name,_TRUNCATE);
	SetStrVal(VarId,Dir);

	return Err;
}

#define IdMsgBox 1
#define IdYesNoBox 2
#define IdStatusBox 3

int MessageCommand(int BoxId, LPWORD Err)
{
	TStrVal Str1, Str2;

	*Err = 0;
	GetStrVal(Str1,Err);
	GetStrVal(Str2,Err);
	if ((*Err==0) && (GetFirstChar()!=0))
		*Err = ErrSyntax;
	if (*Err!=0) return 0;

	if (BoxId==IdMsgBox)
		OpenMsgDlg(Str1,Str2,FALSE);
	else if (BoxId==IdYesNoBox)
		return OpenMsgDlg(Str1,Str2,TRUE);
	else if (BoxId==IdStatusBox)
		OpenStatDlg(Str1,Str2);
	return 0;
}

WORD TTLMessageBox()
{
	WORD Err;

	MessageCommand(IdMsgBox, &Err);
	return Err;
}

WORD TTLNext()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;
	return NextLoop();
}

WORD TTLPause()
{
	int TimeOut;
	WORD Err;

	Err = 0;
	GetIntVal(&TimeOut,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (TimeOut>0)
	{
		TTLStatus = IdTTLPause;
		TimeLimit = (DWORD)(TimeOut*1000);
		TimeStart = GetTickCount();
		SetTimer(HMainWin, IdTimeOutTimer, 50, NULL);
	}
	return Err;
}

// add 'mpause' command
// SYNOPSIS: mpause millisecoand
// DESCRIPTION: This command would sleep TeraTerm macro program by specified millisecond time.
// (2006.2.10 yutaka)
WORD TTLMilliPause()
{
	int TimeOut;
	WORD Err;

	Err = 0;
	GetIntVal(&TimeOut,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (TimeOut > 0) {
		Sleep(TimeOut);
	}

	return Err;
}


// add 'random' command
// SYNOPSIS: random <intvar> <value>
// DESCRIPTION: 
// This command generates the random value from 0 to <value> and
// stores the value to <intvar>.
// (2006.2.11 yutaka)
WORD TTLRecvRandom()
{
	static int srand_init = 0;
	WORD VarId, Err;
	int MaxNum, Num;
	double d;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetIntVal(&MaxNum,&Err);

	if ( ((Err==0) && (GetFirstChar()!=0)) || MaxNum <= 0)
		Err = ErrSyntax;
	if (Err!=0) return Err;

	// 乱数 0 〜 <intvar> を生成する
	if (srand_init == 0) {
		srand_init = 1;
		// VS2005では time_t が64ビット化されたので、キャストを追加。(2006.2.18 yutaka)
		srand((unsigned int)time(NULL));
	}
	//d = (1.0 / (RAND_MAX + 1.0)) * (rand() + 0.5);
	d = rand();
	d = (rand() / (double)RAND_MAX) * MaxNum;
	Num = (int)d;

	SetIntVal(VarId,Num);

	return Err;
}


WORD TTLRecvLn()
{
	TStrVal Str;
	WORD ValType, VarId;
	int TimeOut;

	if (GetFirstChar()!=0)
		return ErrSyntax;
	if (! Linked)
		return ErrLinkFirst;

	ClearWait();

	Str[0] = 0x0a;
	Str[1] = 0;
	SetWait(1,Str);
	SetInputStr("");
	SetResult(1);
	TTLStatus = IdTTLWaitNL;
	TimeOut = 0;
	if (CheckVar("timeout",&ValType,&VarId) &&
	    (ValType==TypInteger))
		TimeOut = CopyIntVal(VarId);

	if (TimeOut>0)
	{
		TimeLimit = (DWORD)(TimeOut*1000);
		TimeStart = GetTickCount();
		SetTimer(HMainWin, IdTimeOutTimer, 50, NULL);
	}

	return 0;
}

WORD TTLReturn()
{
	if (GetFirstChar()==0)
		return ReturnFromSub();
	else
		return ErrSyntax;
}

WORD TTLSend()
{
	TStrVal Str;
	WORD Err, ValType;
	int Val;
	BOOL EndOfLine;

	if (! Linked)
		return ErrLinkFirst;

	EndOfLine = FALSE;
	do {
		if (GetString(Str,&Err))
		{
			if (Err!=0) return Err;
			DDEOut(Str);
		}
		else if (GetExpression(&ValType,&Val,&Err))
		{
			if (Err!=0) return Err;
			switch (ValType) {
				case TypInteger: DDEOut1Byte(LOBYTE(Val)); break;
				case TypString: DDEOut(StrVarPtr((WORD)Val)); break;
				default:
					return ErrTypeMismatch;
			}
		}
		else
			EndOfLine = TRUE;
	} while (! EndOfLine);
	return 0;
}

WORD TTLSendFile()
{
	TStrVal Str;
	WORD Err;
	int BinFlag;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&BinFlag,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	SetFile(Str);
	SetBinary(BinFlag);
	return SendCmnd(CmdSendFile,IdTTLWaitCmndEnd);
}

WORD TTLSendKCode()
{
	TStrVal Str;
	WORD Err;
	int KCode, Count;

	Err = 0;
	GetIntVal(&KCode,&Err);
	GetIntVal(&Count,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Word2HexStr((WORD)KCode,Str);
	Word2HexStr((WORD)Count,&Str[4]);
	SetFile(Str);
	return SendCmnd(CmdSendKCode,0);
}

WORD TTLSendLn()
{
	WORD Err;
	char Str[3];

	Err = TTLSend();
	if (Err==0)
	{
		Str[0] = 0x0D;
		Str[1] = 0x0A;
		Str[2] = 0;
		if (Linked)
			DDEOut(Str);
		else
			Err = ErrLinkFirst;
	}
	return Err;
}

WORD TTLSetDate()
{
	WORD Err;
	TStrVal Str;
	int y, m, d;
#ifdef TERATERM32
	SYSTEMTIME Time;
#else
	union REGS regs;
#endif

	Err = 0;
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Str[4] = 0;
	if (sscanf(Str,"%u",&y)!=1) return 0;
	Str[7] = 0;
	if (sscanf(&(Str[5]),"%u",&m)!=1) return 0;
	Str[10] = 0;
	if (sscanf(&(Str[8]),"%u",&d)!=1) return 0;
#ifdef TERATERM32
	GetLocalTime(&Time);
	Time.wYear = y;
	Time.wMonth = m;
	Time.wDay = d;
	SetLocalTime(&Time);
#else
	regs.h.ah = 0x2b;
	regs.w.cx = y;
	regs.h.dh = m;
	regs.h.dl = d;
	int86(0x21,&regs,&regs);
#endif
	return Err;
}

WORD TTLSetDir()
{
	WORD Err;
	TStrVal Str;

	Err = 0;
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	TTMSetDir(Str);

	return Err;
}

WORD TTLSetDlgPos()
{
	WORD Err;
	int x, y;

	Err = 0;
	GetIntVal(&x,&Err);
	GetIntVal(&y,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	SetDlgPos(x,y);
	return Err;
}

/*WORD TTLSetEnv()
{
  WORD Err;
  TStrVal Str, Str2;

  Err = 0;
  GetStrVal(Str,&Err);
  GetStrVal(Str2,&Err);
  if ((Err==0) && (GetFirstChar()!=0))
    Err = ErrSyntax;
  if (Err!=0) return Err;

  setenv(Str,Str2,1);
  return Err;
} */

WORD TTLSetExitCode()
{
	WORD Err;
	int Val;

	Err = 0;
	GetIntVal(&Val,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	ExitCode = Val;
	return Err;
}

WORD TTLSetSync()
{
	WORD Err;
	int Val;

	Err = 0;
	GetIntVal(&Val,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;

	if (Val==0)
		SetSync(FALSE);
	else
		SetSync(TRUE);

	return 0;
}

WORD TTLSetTime()
{
	WORD Err;
	TStrVal Str;
	int h, m, s;
#ifdef TERATERM32
	SYSTEMTIME Time;
#else
	union REGS regs;
#endif

	Err = 0;
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Str[2] = 0;
	if (sscanf(Str,"%u",&h)!=1) return 0;
	Str[5] = 0;
	if (sscanf(&(Str[3]),"%u",&m)!=1) return 0;
	Str[8] = 0;
	if (sscanf(&(Str[6]),"%u",&s)!=1) return 0;

#ifdef TERATERM32
	GetLocalTime(&Time);
	Time.wHour = h;
	Time.wMinute = m;
	Time.wSecond = s;
	SetLocalTime(&Time);
#else
	regs.h.ah = 0x2d;
	regs.h.ch = h;
	regs.h.cl = m;
	regs.h.dh = s;
	regs.h.dl = 0;
	int86(0x21,&regs,&regs);
#endif

	return Err;
}

WORD TTLShow()
{
	WORD Err;
	int Val;

	Err = 0;
	GetIntVal(&Val,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	if (Val==0)
		ShowWindow(HMainWin,SW_MINIMIZE);
	else if (Val>0)
		ShowWindow(HMainWin,SW_RESTORE);
	else
		ShowWindow(HMainWin,SW_HIDE);
	return Err;
}

// 'sprintf': Format a string in the style of sprintf
//
// (2007.5.1 yutaka)
// (2007.5.3 maya)
WORD TTLSprintf()
{
	TStrVal Fmt;
	int Num;
	TStrVal Str;
	WORD Err = 0, TmpErr;
	char buf[MaxStrLen];
	char *p, subFmt[MaxStrLen], buf2[MaxStrLen];

	enum arg_type {
		INTEGER,
		DOUBLE,
		STRING,
		NONE
	};
	enum arg_type type;

	int r;
	unsigned char *start, *range, *end;
	regex_t* reg;
	OnigErrorInfo einfo;
	OnigRegion *region;
	UChar* pattern, * str;

	pattern = (UChar* )"^%[-+0 #]*(?:[1-9][0-9]*)?(?:\\.[0-9]*)?$";

	r = onig_new(&reg, pattern, pattern + strlen(pattern),
	             ONIG_OPTION_DEFAULT, ONIG_ENCODING_ASCII, ONIG_SYNTAX_DEFAULT,
	             &einfo);
	if (r != ONIG_NORMAL) {
		char s[ONIG_MAX_ERROR_MESSAGE_LEN];
		onig_error_code_to_str(s, r, &einfo);
		fprintf(stderr, "ERROR: %s\n", s);
		LockVar();
		SetResult(-1);
		UnlockVar();
		goto exit2;
	}

	region = onig_region_new();

	GetStrVal(Fmt, &Err);
	if (Err!=0) {
		LockVar();
		SetResult(1);
		UnlockVar();
		goto exit2;
	}

	p = Fmt;
	memset(buf, 0, sizeof(buf));
	memset(subFmt, 0, sizeof(subFmt));
	while(*p != '\0') {
		if (strlen(subFmt)>0) {
			type = NONE;
			switch (*p) {
				case '%':
					if (strlen(subFmt) == 1) { // "%%" -> "%"
						strncat_s(buf, sizeof(buf), "%", _TRUNCATE);
						memset(subFmt, 0, sizeof(subFmt));
					}
					else {
						// 一つ手前までをそのまま buf に格納
						strncat_s(buf, sizeof(buf), subFmt, _TRUNCATE);
						// 仕切り直し
						memset(subFmt, 0, sizeof(subFmt));
						strncat_s(subFmt, sizeof(subFmt), p, 1);
					}
					break;

				case 'c':
				case 'd':
				case 'i':
				case 'o':
				case 'u':
				case 'x':
				case 'X':
					type = INTEGER;

				case 'e':
				case 'E':
				case 'f':
				case 'g':
				case 'G':
				case 'a':
				case 'A':
					if (type == NONE) {
						type = DOUBLE;
					}

				case 's':
					if (type == NONE) {
						type = STRING;
					}

					// "%" と *p の間が正しいかチェック
					str = (UChar* )subFmt;
					end   = str + strlen(subFmt);
					start = str;
					range = end;
					r = onig_search(reg, str, end, start, range, region,
					                ONIG_OPTION_NONE);
					if (r != 0) {
						LockVar();
						SetResult(2);
						UnlockVar();
						Err = ErrSyntax;
						goto exit1;
					}

					strncat_s(subFmt, sizeof(subFmt), p, 1);

					if (type == STRING) {
						// 文字列として読めるかトライ
						TmpErr = 0;
						GetStrVal(Str, &TmpErr);
						if (TmpErr == 0) {
							_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, Str);
						}
						else {
							LockVar();
							SetResult(3);
							UnlockVar();
							Err = TmpErr;
							goto exit1;
						}
					}
					else {
						// 数値として読めるかトライ
						TmpErr = 0;
						GetIntVal(&Num, &TmpErr);
						if (TmpErr == 0) {
							if (type == INTEGER) {
								_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, Num);
							}
							else {
								_snprintf_s(buf2, sizeof(buf2), _TRUNCATE, subFmt, (double)Num);
							}
						}
						else {
							LockVar();
							SetResult(3);
							UnlockVar();
							Err = TmpErr;
							goto exit1;
						}
					}

					strncat_s(buf, sizeof(buf), buf2, _TRUNCATE);
					memset(subFmt, 0, sizeof(subFmt));
					break;

				default:
					strncat_s(subFmt, sizeof(subFmt), p, 1);
			}
		}
		else if (*p == '%') {
			strncat_s(subFmt, sizeof(subFmt), p, 1);
		}
		else if (strlen(buf) < sizeof(buf)-1) {
			strncat_s(buf, sizeof(buf), p, 1);
		}
		else {
			break;
		}
		p++;
	}
	if (strlen(subFmt) > 0) {
		strncat_s(buf, sizeof(buf), subFmt, _TRUNCATE);
	}

	// マッチした行を inputstr へ格納する
	LockVar();
	SetInputStr(buf);  // ここでバッファがクリアされる
	SetResult(0);
	UnlockVar();

exit1:
	onig_region_free(region, 1);
exit2:
	onig_free(reg);
	onig_end();

	return Err;
}

WORD TTLStatusBox()
{
	WORD Err;

	MessageCommand(IdStatusBox, &Err);
	return Err;
}

WORD TTLStr2Code()
{
	WORD VarId, Err;
	TStrVal Str;
	int Len, c, i;
	unsigned int Num;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	Len = strlen(Str);
	if (Len > sizeof(Num))
		c = sizeof(Num);
	else
		c = Len;
	Num = 0;
	for (i=c; i>=1; i--)
		Num = Num*256 + (BYTE)Str[Len-i];
	SetIntVal(VarId,Num);

	return Err;
}

WORD TTLStr2Int()
{
	WORD VarId, Err;
	TStrVal Str;
	int Num;

	Err = 0;
	GetIntVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	// C言語では16進は0xで始まるが、TTL仕様では $ で始まるため、後者もサポートする。
	if (Str[0] == '$') {
		memmove_s(Str + 2, sizeof(Str) - 2, Str + 1, strlen(Str));
		Str[0] = '0';
		Str[1] = 'x';
	}

	// '%d'から'%i'へ変更により、10進以外の数値を変換できるようにする。 (2007.5.1 yutaka)
	// 下位互換性のため10進と16進のみのサポートとする。(2007.5.2 yutaka)
	// 10 : decimal
	// 0x10, $10: hex
	if (Str[0] == '0' && tolower(Str[1]) == 'x') {
		if (sscanf(Str,"%i",&Num)!=1)
		{
			Num = 0;
			SetResult(0);
		}
		else {
			SetResult(1);
		}
	} else {
		if (sscanf(Str,"%d",&Num)!=1)
		{
			Num = 0;
			SetResult(0);
		}
		else {
			SetResult(1);
		}
	}
	SetIntVal(VarId,Num);

	return Err;
}

WORD TTLStrCompare()
{
	TStrVal Str1, Str2;
	WORD Err;
	int i;

	Err = 0;
	GetStrVal(Str1,&Err);
	GetStrVal(Str2,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	i = strcmp(Str1,Str2);
	if (i<0)
		i = -1;
	else if (i>0)
		i = 1;
	SetResult(i);
	return Err;
}

WORD TTLStrConcat()
{
	WORD VarId, Err;
	TStrVal Str;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	strncat_s(StrVarPtr(VarId),MaxStrLen,Str,_TRUNCATE);
	return Err;
}

WORD TTLStrCopy()
{
	WORD Err, VarId;
	int From, Len, SrcLen;
	TStrVal Str;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&From,&Err);
	GetIntVal(&Len,&Err);
	GetStrVar(&VarId,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (From<1) From = 1;
	SrcLen = strlen(Str)-From+1;
	if (Len > SrcLen) Len = SrcLen;
	if (Len < 0) Len = 0;
	memcpy(StrVarPtr(VarId),&(Str[From-1]),Len);
	StrVarPtr(VarId)[Len] = 0;
	return Err;
}

WORD TTLStrLen()
{
	WORD Err;
	TStrVal Str;

	Err = 0;
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;
	SetResult(strlen(Str));
	return Err;
}

WORD TTLStrScan()
{
	WORD Err;
	int Len1, Len2, i, j;
	TStrVal Str1, Str2;

	Err = 0;
	GetStrVal(Str1,&Err);
	GetStrVal(Str2,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	Len1 = strlen(Str1);
	Len2 = strlen(Str2);
	if ((Len1==0) || (Len2==0))
	{
		SetResult(0);
		return Err;
	}

	i = 0;
	j = 0;
	do {
		if (Str1[i]==Str2[j])
		{
			j++;
			i++;
		}
		else if (j==0)
			i++;
		else
			j = 0;
	} while ((i<Len1) && (j<Len2));
	if (j==Len2)
		SetResult(i-Len2+1);
	else
		SetResult(0);
	return Err;
}

WORD TTLTestLink()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (! Linked)
		SetResult(0);
	else if (ComReady==0)
		SetResult(1);
	else
		SetResult(2);

	return 0;
}

// added (2007.7.12 maya)
WORD TTLToLower()
{
	WORD Err, VarId;
	TStrVal Str;
	int i=0;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	while (Str[i] != 0) {
		if(_ismbblead(Str[i])) {
			i = i + 2;
			continue;
		}
		if (Str[i] >= 'A' && Str[i] <= 'Z') {
			Str[i] = Str[i] + 0x20;
		}
		i++;
	}

	strncpy_s(StrVarPtr(VarId), MaxStrLen, Str, _TRUNCATE);
	return Err;
}

// added (2007.7.12 maya)
WORD TTLToUpper()
{
	WORD Err, VarId;
	TStrVal Str;
	int i=0;

	Err = 0;
	GetStrVar(&VarId,&Err);
	GetStrVal(Str,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	while (Str[i] != 0) {
		if(_ismbblead(Str[i])) {
			i = i + 2;
			continue;
		}
		if (Str[i] >= 'a' && Str[i] <= 'z') {
			Str[i] = Str[i] - 0x20;
		}
		i++;
	}

	strncpy_s(StrVarPtr(VarId), MaxStrLen, Str, _TRUNCATE);
	return Err;
}

WORD TTLUnlink()
{
	if (GetFirstChar()!=0)
		return ErrSyntax;

	if (Linked)
	{
		EndDDE();
	}
	return 0;
}



WORD TTLWait(BOOL Ln)
{
	TStrVal Str;
	WORD Err, ValType, VarId;
	int i, Val;
	BOOL NoMore;
	int TimeOut;

	ClearWait();

	i = 0;
	do {
		Err = 0;
		Str[0] = 0;
		NoMore = FALSE;
		if (! GetString(Str,&Err))
		{
			if (GetExpression(&ValType,&Val,&Err))
			{
				if (Err==0)
				{
					if (ValType==TypString)
						strncpy_s(Str, sizeof(Str),StrVarPtr((WORD)Val), _TRUNCATE);
					else
						Err=ErrTypeMismatch;
				}
			}
			else
				NoMore = TRUE;
		}

		if ((Err==0) && (strlen(Str)>0) && (i<10))
		{
			i++;
			SetWait(i,Str);
		}
	} while ((Err==0) && (i<10) && !NoMore);

	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;

	if (! Linked)
		Err = ErrLinkFirst;

	if ((Err==0) && (i>0))
	{
		if (Ln)
			TTLStatus = IdTTLWaitLn;
		else
			TTLStatus = IdTTLWait;
		TimeOut = 0;
		if (CheckVar("timeout",&ValType,&VarId) &&
		    (ValType==TypInteger))
			TimeOut = CopyIntVal(VarId);

		if (TimeOut>0)
		{
			TimeLimit = (DWORD)(TimeOut*1000);
			TimeStart = GetTickCount();
			SetTimer(HMainWin, IdTimeOutTimer, 50, NULL);
		}
	}
	else
		ClearWait();

	return Err;
}


// 'waitregex'(wait regular expression): wait command with regular expression
//
// This command has almost same function of 'wait' command. Additionally 'waitregex' can search 
// the keyword with regular expression. TeraTerm uses a regex library that is called 'Oniguruma'.
// cf. http://www.geocities.jp/kosako3/oniguruma/
//
// (2005.10.5 yutaka)
WORD TTLWaitRegex(BOOL Ln)
{
	WORD ret;

	ret = TTLWait(Ln);

	RegexActionType = REGEX_WAIT; // regex enabled

	return (ret);
}



WORD TTLWaitEvent()
{
	WORD Err, ValType, VarId;
	int TimeOut;

	Err = 0;
	GetIntVal(&WakeupCondition,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	WakeupCondition &= 15;
	TimeOut = 0;
	if (CheckVar("timeout",&ValType,&VarId) &&
	    (ValType==TypInteger))
		TimeOut = CopyIntVal(VarId);

	if (TimeOut>0)
	{
		TimeLimit = (DWORD)(TimeOut*1000);
		TimeStart = GetTickCount();
		SetTimer(HMainWin, IdTimeOutTimer, 50, NULL);
	}

	TTLStatus = IdTTLSleep;
	return Err;
}

WORD TTLWaitRecv()
{
	TStrVal Str;
	WORD Err;
	int Pos, Len, TimeOut;
	WORD VarType, VarId;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&Len,&Err);
	GetIntVal(&Pos,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if ((Err==0) && (! Linked))
		Err = ErrLinkFirst;
	if (Err!=0) return Err;
	SetInputStr("");
	SetWait2(Str,Len,Pos);

	TTLStatus = IdTTLWait2;
	TimeOut = 0;
	if (CheckVar("timeout",&VarType,&VarId) &&
	    (VarType==TypInteger))
		TimeOut = CopyIntVal(VarId);
	if (TimeOut>0)
	{
		TimeLimit = (DWORD)(TimeOut*1000);
		TimeStart = GetTickCount();
		SetTimer(HMainWin, IdTimeOutTimer, 50, NULL);
	}
	return Err;
}

WORD TTLWhile()
{
	WORD Err;
	int Val;

	Err = 0;
	GetIntVal(&Val,&Err);
	if ((Err==0) && (GetFirstChar()!=0))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	if (Val!=0)
		return SetWhileLoop();
	else
		EndWhileLoop();
	return Err;
}

WORD TTLXmodemRecv()
{
	TStrVal Str;
	WORD Err;
	int BinFlag, XOption;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&BinFlag,&Err);
	GetIntVal(&XOption,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;

	if (Err!=0) return Err;

	SetFile(Str);
	SetBinary(BinFlag);
	SetXOption(XOption);
	return SendCmnd(CmdXmodemRecv,IdTTLWaitCmndResult);
}

WORD TTLXmodemSend()
{
	TStrVal Str;
	WORD Err;
	int XOption;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&XOption,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetFile(Str);
	SetXOption(XOption);
	return SendCmnd(CmdXmodemSend,IdTTLWaitCmndResult);
}

WORD TTLYesNoBox()
{
	WORD Err;
	int YesNo;

	YesNo = MessageCommand(IdYesNoBox, &Err);
	if (Err!=0) return Err;
	if (YesNo==IDOK)
		YesNo = 1;	// Yes
	else
		YesNo = 0;	// No
	SetResult(YesNo);
	return Err;
}

WORD TTLZmodemSend()
{
	TStrVal Str;
	WORD Err;
	int BinFlag;

	Err = 0;
	GetStrVal(Str,&Err);
	GetIntVal(&BinFlag,&Err);
	if ((Err==0) &&
	    ((strlen(Str)==0) || (GetFirstChar()!=0)))
		Err = ErrSyntax;
	if (Err!=0) return Err;

	SetFile(Str);
	SetBinary(BinFlag);
	return SendCmnd(CmdZmodemSend,IdTTLWaitCmndResult);
}

int ExecCmnd()
{
	WORD WId, Err;
	BOOL StrConst, E;
	TStrVal Str;
	TName Cmnd;
	WORD ValType, VarType, VarId;
	int Val;

	Err = 0;

	if (EndWhileFlag>0)
	{
		if (! GetReservedWord(&WId))
			;
		else if (WId==RsvWhile)
			EndWhileFlag++;
		else if (WId==RsvEndWhile)
			EndWhileFlag--;
		return 0;
	}

	if (BreakFlag>0)
	{
		if (! GetReservedWord(&WId))
			;
		else if ((WId==RsvIf) && (CheckThen(&Err)))
			IfNest++;
		else if (WId==RsvEndIf)
		{
			if (IfNest<1)
				Err = ErrInvalidCtl;
			else
				IfNest--;
		}
		else if (WId==RsvFor || WId==RsvWhile)
			BreakFlag++;
		else if (WId==RsvNext || WId==RsvEndWhile)
			BreakFlag--;
		return Err;
	}

	if (EndIfFlag>0)
	{
		if (! GetReservedWord(&WId))
			;
		else if ((WId==RsvIf) && CheckThen(&Err))
			EndIfFlag++;
		else if (WId==RsvEndIf)
			EndIfFlag--;
		return Err;
	}

	if (ElseFlag>0)
	{
		if (! GetReservedWord(&WId))
			;
		else if ((WId==RsvIf) && CheckThen(&Err))
			EndIfFlag++;
		else if (WId==RsvElse)
			ElseFlag--;
		else if (WId==RsvElseIf)
		{
			if (CheckElseIf(&Err)!=0)
				ElseFlag--;
		}
		else if (WId==RsvEndIf)
		{
			ElseFlag--;
			if (ElseFlag==0)
				IfNest--;
		}
		return Err;
	}

	if (GetReservedWord(&WId)) 
		switch (WId) {
		case RsvBeep:
			Err = TTLBeep(); break;
		case RsvBPlusRecv:
			Err = TTLCommCmd(CmdBPlusRecv,IdTTLWaitCmndResult); break;
		case RsvBPlusSend:
			Err = TTLCommCmdFile(CmdBPlusSend,IdTTLWaitCmndResult); break;
		case RsvBreak:
			Err = TTLBreak(); break;
		case RsvCall:
			Err = TTLCall(); break;
		case RsvChangeDir:
			Err = TTLCommCmdFile(CmdChangeDir,0); break;
		case RsvClearScreen:
			Err = TTLCommCmdInt(CmdClearScreen,0); break;
		case RsvClipb2Var:
			Err = TTLClipb2Var(); break;    // add 'clipb2var' (2006.9.17 maya)
		case RsvCloseSBox:
			Err = TTLCloseSBox(); break;
		case RsvCloseTT:
			Err = TTLCloseTT(); break;
		case RsvCLShift:
			Err = TTLCircularLeftShift(); break;
		case RsvCode2Str:
			Err = TTLCode2Str(); break;
		case RsvConnect:
			Err = TTLConnect(); break;
		case RsvCRShift:
			Err = TTLCircularRightShift(); break;
		case RsvDelPassword:
			Err = TTLDelPassword(); break;
		case RsvDisconnect:
			Err = TTLCommCmd(CmdDisconnect,0); break;
		case RsvElse:
			Err = TTLElse(); break;
		case RsvElseIf:
			Err = TTLElseIf(); break;
		case RsvEnableKeyb:
			Err = TTLCommCmdBin(CmdEnableKeyb,0); break;
		case RsvEnd:
			Err = TTLEnd(); break;
		case RsvEndIf:
			Err = TTLEndIf(); break;
		case RsvEndWhile:
			Err = TTLEndWhile(); break;
		case RsvExec:
			Err = TTLExec(); break;
		case RsvExecCmnd:
			Err = TTLExecCmnd(); break;
		case RsvExit:
			Err = TTLExit(); break;
		case RsvFileClose:
			Err = TTLFileClose(); break;
		case RsvFileConcat:
			Err = TTLFileConcat(); break;
		case RsvFileCopy:
			Err = TTLFileCopy(); break;
		case RsvFileCreate:
			Err = TTLFileCreate(); break;
		case RsvFileDelete:
			Err = TTLFileDelete(); break;
		case RsvFileMarkPtr:
			Err = TTLFileMarkPtr(); break;
		case RsvFileOpen:
			Err = TTLFileOpen(); break;
		case RsvFileReadln:
			Err = TTLFileReadln(); break;
		case RsvFileRead:
			Err = TTLFileRead(); break;    // add 'fileread'
		case RsvFileRename:
			Err = TTLFileRename(); break;
		case RsvFileSearch:
			Err = TTLFileSearch(); break;
		case RsvFileSeek:
			Err = TTLFileSeek(); break;
		case RsvFileSeekBack:
			Err = TTLFileSeekBack(); break;
		case RsvFileStrSeek:
			Err = TTLFileStrSeek(); break;
		case RsvFileStrSeek2:
			Err = TTLFileStrSeek2(); break;
		case RsvFileWrite:
			Err = TTLFileWrite(); break;
		case RsvFileWriteLn:
			Err = TTLFileWriteLn(); break;
		case RsvFindClose:
			Err = TTLFindClose(); break;
		case RsvFindFirst:
			Err = TTLFindFirst(); break;
		case RsvFindNext:
			Err = TTLFindNext(); break;
		case RsvFlushRecv:
			Err = TTLFlushRecv(); break;
		case RsvFor:
			Err = TTLFor(); break;
		case RsvGetDate:
			Err = TTLGetDate(); break;
		case RsvGetDir:
			Err = TTLGetDir(); break;
		case RsvGetEnv:
			Err = TTLGetEnv(); break;
		case RsvGetPassword:
			Err = TTLGetPassword(); break;
		case RsvGetTime:
			Err = TTLGetTime(); break;
		case RsvGetTitle:
			Err = TTLGetTitle(); break;
		case RsvGoto:
			Err = TTLGoto(); break;
		case RsvIfDefined:
			Err = TTLIfDefined(); break;
		case RsvIf:
			Err = TTLIf(); break;
		case RsvInclude:
			Err = TTLInclude(); break;
		case RsvInputBox:
			Err = TTLInputBox(FALSE); break;
		case RsvInt2Str:
			Err = TTLInt2Str(); break;
		case RsvKmtFinish:
			Err = TTLCommCmd(CmdKmtFinish,IdTTLWaitCmndResult); break;
		case RsvKmtGet:
			Err = TTLCommCmdFile(CmdKmtGet,IdTTLWaitCmndResult); break;
		case RsvKmtRecv:
			Err = TTLCommCmd(CmdKmtRecv,IdTTLWaitCmndResult); break;
		case RsvKmtSend:
			Err = TTLCommCmdFile(CmdKmtSend,IdTTLWaitCmndResult); break;
		case RsvLoadKeyMap:
			Err = TTLCommCmdFile(CmdLoadKeyMap,0); break;
		case RsvLogClose:
			Err = TTLCommCmd(CmdLogClose,0); break;
		case RsvLogOpen:
			Err = TTLLogOpen(); break;
		case RsvLogPause:
			Err = TTLCommCmd(CmdLogPause,0); break;
		case RsvLogStart:
			Err = TTLCommCmd(CmdLogStart,0); break;
		case RsvLogWrite:
			Err = TTLCommCmdFile(CmdLogWrite,0); break;
		case RsvMakePath:
			Err = TTLMakePath(); break;
		case RsvMessageBox:
			Err = TTLMessageBox(); break;
		case RsvNext:
			Err = TTLNext(); break;
		case RsvPasswordBox:
			Err = TTLInputBox(TRUE); break;
		case RsvPause:
			Err = TTLPause(); break;
		case RsvMilliPause:
			Err = TTLMilliPause(); break;    // add 'mpause'
		case RsvQuickVANRecv:
			Err = TTLCommCmd(CmdQVRecv,IdTTLWaitCmndResult); break;
		case RsvQuickVANSend:
			Err = TTLCommCmdFile(CmdQVSend,IdTTLWaitCmndResult); break;
		case RsvRandom:
			Err = TTLRecvRandom(); break;   // add 'random'
		case RsvRecvLn:
			Err = TTLRecvLn(); break;
		case RsvRestoreSetup:
			Err = TTLCommCmdFile(CmdRestoreSetup,0); break;
		case RsvReturn:
			Err = TTLReturn(); break;
		case RsvSend:
			Err = TTLSend(); break;
		case RsvSendBreak:
			Err = TTLCommCmd(CmdSendBreak,0); break;
		case RsvSendFile:
			Err = TTLSendFile(); break;
		case RsvSendKCode:
			Err = TTLSendKCode(); break;
		case RsvSendLn:
			Err = TTLSendLn(); break;
		case RsvSetDate:
			Err = TTLSetDate(); break;
		case RsvSetDir:
			Err = TTLSetDir(); break;
		case RsvSetDlgPos:
			Err = TTLSetDlgPos(); break;
		case RsvSetEcho:
			Err = TTLCommCmdBin(CmdSetEcho,0); break;
		case RsvSetExitCode:
			Err = TTLSetExitCode(); break;
		case RsvSetSync:
			Err = TTLSetSync(); break;
		case RsvSetTime:
			Err = TTLSetTime(); break;
		case RsvSetTitle:
			Err = TTLCommCmdFile(CmdSetTitle,0); break;
		case RsvShow:
			Err = TTLShow(); break;
		case RsvShowTT:
			Err = TTLCommCmdInt(CmdShowTT,0); break;
		case RsvSprintf:
			Err = TTLSprintf(); break;
		case RsvStatusBox:
			Err = TTLStatusBox(); break;
		case RsvStr2Code:
			Err = TTLStr2Code(); break;
		case RsvStr2Int:
			Err = TTLStr2Int(); break;
		case RsvStrCompare:
			Err = TTLStrCompare(); break;
		case RsvStrConcat:
			Err = TTLStrConcat(); break;
		case RsvStrCopy:
			Err = TTLStrCopy(); break;
		case RsvStrLen:
			Err = TTLStrLen(); break;
		case RsvStrScan:
			Err = TTLStrScan(); break;
		case RsvTestLink:
			Err = TTLTestLink(); break;
		case RsvToLower:
			Err = TTLToLower(); break;    // add 'tolower' (2007.7.12 maya)
		case RsvToUpper:
			Err = TTLToUpper(); break;    // add 'toupper' (2007.7.12 maya)
		case RsvUnlink:
			Err = TTLUnlink(); break;
		case RsvVar2Clipb:
			Err = TTLVar2Clipb(); break;    // add 'var2clipb' (2006.9.17 maya)
		case RsvWaitRegex:
			Err = TTLWaitRegex(FALSE); break;    // add 'waitregex' (2005.10.5 yutaka)
		case RsvWait:
			Err = TTLWait(FALSE); break;
		case RsvWaitEvent:
			Err = TTLWaitEvent(); break;
		case RsvWaitLn:
			Err = TTLWait(TRUE); break;
		case RsvWaitRecv:
			Err = TTLWaitRecv(); break;
		case RsvWhile:
			Err = TTLWhile(); break;
		case RsvXmodemRecv:
			Err = TTLXmodemRecv(); break;
		case RsvXmodemSend:
			Err = TTLXmodemSend(); break;
		case RsvYesNoBox:
			Err = TTLYesNoBox(); break;
		case RsvZmodemRecv:
			Err = TTLCommCmd(CmdZmodemRecv,IdTTLWaitCmndResult); break;
		case RsvZmodemSend:
			Err = TTLZmodemSend(); break;
		default:
			Err = ErrSyntax;
		}
	else if (GetIdentifier(Cmnd))
	{
		if (GetFirstChar()=='=')
		{
			StrConst = GetString(Str,&Err);
			if (StrConst)
				ValType = TypString;
			else
				if (! GetOrResult(&ValType,&Val,&Err))
					Err = ErrSyntax;

			if (Err==0)
			{
				if (CheckVar(Cmnd,&VarType,&VarId))
				{
					if (VarType==ValType)
						switch (ValType) {
						case TypInteger: SetIntVal(VarId,Val); break;
						case TypString:
							if (StrConst)
								SetStrVal(VarId,Str);
							else
							// StrVarPtr の返り値が TStrVal のポインタであることを期待してサイズを固定
							// (2007.6.23 maya)
								strncpy_s(StrVarPtr(VarId),MaxStrLen,StrVarPtr((WORD)Val),_TRUNCATE);
							break;
						}
					else
						Err = ErrTypeMismatch;
				}
				else {
					switch (ValType) {
					case TypInteger: E = NewIntVar(Cmnd,Val); break;
					case TypString:
						if (StrConst)
							E = NewStrVar(Cmnd,Str);
						else
							E = NewStrVar(Cmnd,StrVarPtr((WORD)Val));
						break;
					default: 
						E = FALSE;
					}
					if (! E) Err = ErrTooManyVar;
				}
				if ((Err==0) && (GetFirstChar()!=0))
					Err = ErrSyntax;
			}
		}
		else Err = ErrSyntax;
	}
	else
		Err = ErrSyntax;

	return Err;
}

void Exec()
{
	WORD Err;

	// ParseAgain is set by 'ExecCmnd'
	if (! ParseAgain &&
	    ! GetNewLine())
	{
		TTLStatus = IdTTLEnd;
		return;
	}
	ParseAgain = FALSE;

	LockVar();
	Err = ExecCmnd();
	if (Err>0) DispErr(Err);
	UnlockVar();
}

// 正規表現でマッチした文字列を記録する
// (2005.10.7 yutaka)
void SetMatchStr(PCHAR Str)
{
	WORD VarType, VarId;

	if (CheckVar("matchstr",&VarType,&VarId) &&
	    (VarType==TypString))
		SetStrVal(VarId,Str);
}

// 正規表現でグループマッチした文字列を記録する
// (2005.10.15 yutaka)
void SetGroupMatchStr(int no, PCHAR Str)
{
	WORD VarType, VarId;
	char buf[128];

	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "groupmatchstr%d", no);

	if (CheckVar(buf,&VarType,&VarId) &&
	    (VarType==TypString))
		SetStrVal(VarId,Str);
}

void SetInputStr(PCHAR Str)
{
	WORD VarType, VarId;

	if (CheckVar("inputstr",&VarType,&VarId) &&
	    (VarType==TypString))
		SetStrVal(VarId,Str);
}

void SetResult(int ResultCode)
{
  WORD VarType, VarId;

	if (CheckVar("result",&VarType,&VarId) &&
	    (VarType==TypInteger))
		SetIntVal(VarId,ResultCode);
}

BOOL CheckTimeout()
{
	BOOL ret;
	DWORD TimeUp = (TimeStart+TimeLimit);

	if (TimeUp > TimeStart) {
		ret = (GetTickCount() > TimeUp);
	}
	else { // for DWORD overflow (49.7 days)
		DWORD TimeTmp = GetTickCount();
		ret = (TimeUp < TimeTmp && TimeTmp >= TimeStart);
	}
	return ret;
}

BOOL TestWakeup(int Wakeup)
{
	return ((Wakeup & WakeupCondition) != 0);
}

void SetWakeup(int Wakeup)
{
	WakeupCondition = Wakeup;
}
