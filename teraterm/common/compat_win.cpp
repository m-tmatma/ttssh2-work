/*
 * (C) 20018-2019 TeraTerm Project
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

/* compat_win */

#include <windows.h>
#include <tchar.h>
#include "compat_win.h"

#include "dllutil.h"

LRESULT (WINAPI *pSendDlgItemMessageW)(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL (WINAPI *pModifyMenuW)(HMENU hMnu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
BOOL (WINAPI *pSetWindowTextW)(HWND hWnd, LPCWSTR lpString);
DWORD (WINAPI *pGetPrivateProfileStringW)(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);
UINT (WINAPI *pDragQueryFileW)(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch);
DWORD (WINAPI *pGetFileAttributesW)(LPCWSTR lpFileName);
BOOL (WINAPI *pSetDlgItemTextW)(HWND hDlg, int nIDDlgItem, LPCWSTR lpString);
BOOL (WINAPI *pAlphaBlend)(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);
BOOL (WINAPI *pEnumDisplayMonitors)(HDC,LPCRECT,MONITORENUMPROC,LPARAM);
DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
BOOL (WINAPI *pIsValidDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
UINT (WINAPI *pGetDpiForWindow)(HWND hwnd);
BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
int (WINAPI *pAddFontResourceExA)(LPCSTR name, DWORD fl, PVOID res);
int (WINAPI *pAddFontResourceExW)(LPCWSTR name, DWORD fl, PVOID res);
BOOL (WINAPI *pRemoveFontResourceExA)(LPCSTR name, DWORD fl, PVOID pdv);
BOOL (WINAPI *pRemoveFontResourceExW)(LPCWSTR name, DWORD fl, PVOID pdv);
HRESULT (WINAPI *pGetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);
HMONITOR (WINAPI *pMonitorFromRect)(LPCRECT lprc, DWORD dwFlags);
BOOL (WINAPI *pAdjustWindowRectExForDpi)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);

static const APIInfo Lists_user32[] = {
	{ "SetLayeredWindowAttributes", (void **)&pSetLayeredWindowAttributes },
	{ "SetThreadDpiAwarenessContext", (void **)&pSetThreadDpiAwarenessContext },
	{ "IsValidDpiAwarenessContext", (void **)&pIsValidDpiAwarenessContext },
	{ "GetDpiForWindow", (void **)&pGetDpiForWindow },
	{ "MonitorFromRect", (void **)&pMonitorFromRect },
	{ "AdjustWindowRectExForDpi", (void **)&pAdjustWindowRectExForDpi },
	{ "SetDlgItemTextW", (void **)&pSetDlgItemTextW },
	{ "SetWindowTextW", (void **)&pSetWindowTextW },
	{ "ModifyMenuW", (void **)&pModifyMenuW },
	{ "SendDlgItemMessageW", (void **)&pSendDlgItemMessageW },
	{ NULL, NULL },
};

static const APIInfo Lists_msimg32[] = {
	{ "AlphaBlend", (void **)&pAlphaBlend },
	{ NULL, NULL },
};

static const APIInfo Lists_gdi32[] = {
	{ "AddFontResourceExA", (void **)&pAddFontResourceExA },
	{ "RemoveFontResourceExA", (void **)&pRemoveFontResourceExA },
	{ "AddFontResourceExW", (void **)&pAddFontResourceExW },
	{ "RemoveFontResourceExW", (void **)&pRemoveFontResourceExW },
	{ NULL, NULL },
};

static const APIInfo Lists_Shcore[] = {
	{ "GetDpiForMonitor", (void **)&pGetDpiForMonitor },
	{ NULL, NULL },
};

static const APIInfo Lists_kernel32[] = {
	{ "GetFileAttributesW", (void **)&pGetFileAttributesW },
	{ "GetPrivateProfileStringW", (void **)&pGetPrivateProfileStringW },
	{ NULL, NULL },
};

static const APIInfo Lists_shell32[] = {
	{ "DragQueryFileW", (void **)&pDragQueryFileW },
	{ NULL, NULL },
};

static const DllInfo DllInfos[] = {
	{ _T("user32.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_user32 },
	{ _T("msimg32.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_msimg32 },
	{ _T("gdi32.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_gdi32 },
	{ _T("Shcore.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_Shcore },
	{ _T("kernel32.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_kernel32 },
	{ _T("shell32.dll"), DLL_LOAD_LIBRARY_SYSTEM, DLL_ACCEPT_NOT_EXIST, Lists_shell32 },
	{ NULL, DLL_GET_MODULE_HANDLE, DLL_ACCEPT_NOT_EXIST, NULL },
};

void WinCompatInit()
{
	static BOOL done = FALSE;
	if (done) return;
	done = TRUE;

	DLLGetApiAddressFromLists(DllInfos);
}
