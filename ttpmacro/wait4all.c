/*
 * "wait4all"マクロコマンド用ルーチン
 *
 */
#include <stdio.h>
#include <windows.h>
#include "wait4all.h"

// TODO: 以下の定義は共通ヘッダからincludeすべき
#define MAXNWIN 50
#define RingBufSize (4096*4)

// 共有メモリフォーマット拡張時は、以下の名称を変更すること。
#define TTM_FILEMAPNAME "ttm_memfilemap_1"

// 共有メモリのフォーマット
typedef struct {
	HWND WinList[MAXNWIN];
	int NWin;
	struct mbuf {
		char RingBuf[RingBufSize];
		int RBufStart;
		int RBufPtr;
		int RBufCount;
	} mbufs[MAXNWIN];
} TMacroShmem;

static HANDLE HMap = NULL;
static BOOL FirstInstance = FALSE;
static TMacroShmem *pm = NULL;
static int mindex = -1;
static BOOL QuoteFlag;

// 共有メモリのマッピング
int open_macro_shmem(void)
{
	HMap = CreateFileMapping(
		(HANDLE) 0xFFFFFFFF, NULL, PAGE_READWRITE,
		0, sizeof(TMacroShmem), TTM_FILEMAPNAME);
	if (HMap == NULL)
		return FALSE;

	FirstInstance = (GetLastError() != ERROR_ALREADY_EXISTS);

	pm = (TMacroShmem *)MapViewOfFile(HMap,FILE_MAP_WRITE,0,0,0);
	if (pm == NULL)
		return FALSE;

	memset(pm, 0, sizeof(TMacroShmem));

	return TRUE;
}

void close_macro_shmem(void)
{
	if (pm) {
		UnmapViewOfFile(pm);
		pm = NULL;
	}
	if (HMap) {
		CloseHandle(HMap);
		HMap = NULL;
	}
}

// マクロウィンドウを登録する
int register_macro_window(HWND hwnd)
{
	int i;
	int ret = FALSE;

	for (i = 0 ; i < MAXNWIN ; i++) {
		if (pm->WinList[i] == NULL) {
			pm->NWin++;
			pm->WinList[i] = hwnd;
			mindex = i;   // インデックスを保存
			ret = TRUE;
			break;
		}
	}

	return (ret);
}

// マクロウィンドウを登録解除する
int unregister_macro_window(HWND hwnd)
{
	int i;
	int ret = FALSE;

	for (i = 0 ; i < MAXNWIN ; i++) {
		if (pm->WinList[i] == hwnd) {
			pm->NWin--;
			pm->WinList[i] = NULL;
			ret = TRUE;
			break;
		}
	}

	return (ret);
}

void put_macro_1byte(BYTE b)
{
	char *RingBuf = pm->mbufs[mindex].RingBuf;
	int RBufPtr = pm->mbufs[mindex].RBufPtr;
	int RBufCount = pm->mbufs[mindex].RBufCount;
	int RBufStart = pm->mbufs[mindex].RBufStart;

	RingBuf[RBufPtr] = b;
	RBufPtr++;
	if (RBufPtr>=RingBufSize) {
		RBufPtr = RBufPtr-RingBufSize;
	}
	if (RBufCount>=RingBufSize) {
		RBufCount = RingBufSize;
		RBufStart = RBufPtr;
	}
	else {
		RBufCount++;
	}

	// push back
	pm->mbufs[mindex].RBufPtr = RBufPtr;
	pm->mbufs[mindex].RBufCount = RBufCount;
	pm->mbufs[mindex].RBufStart = RBufStart;
}

int read_macro_1byte(LPBYTE b)
{
	char *RingBuf = pm->mbufs[mindex].RingBuf;
	int RBufPtr = pm->mbufs[mindex].RBufPtr;
	int RBufCount = pm->mbufs[mindex].RBufCount;
	int RBufStart = pm->mbufs[mindex].RBufStart;

	if (RBufCount<=0) {
		return FALSE;
	}

	*b = RingBuf[RBufStart];
	RBufStart++;
	if (RBufStart>=RingBufSize) {
		RBufStart = RBufStart-RingBufSize;
	}
	RBufCount--;
	if (QuoteFlag) {
		*b= *b-1;
		QuoteFlag = FALSE;
	}
	else {
		QuoteFlag = (*b==0x01);
	}

	// push back
	pm->mbufs[mindex].RBufPtr = RBufPtr;
	pm->mbufs[mindex].RBufCount = RBufCount;
	pm->mbufs[mindex].RBufStart = RBufStart;

	return (! QuoteFlag);
}
