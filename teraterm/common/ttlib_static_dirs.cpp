/*
 * Copyright (C) 2020- TeraTerm Project
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

/* C99���ɋL�q�ł���悤��cpp�Ƃ��� */
/* Visual Studio 2005 �� C89 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <crtdbg.h>
#endif
#include <assert.h>
#include <wchar.h>
#include <shlobj.h>
#include <malloc.h>
#include <time.h>

#include "i18n.h"
#include "asprintf.h"
#include "win32helper.h"
#include "codeconv.h"
#include "compat_win.h"
#include "fileread.h"

#include "ttlib.h"

#if defined(__CYGWIN__)
#define _wcsdup(p1) wcsdup(p1)
#endif

/**
 *	�|�[�^�u���łƎw�蓮�삷�邩
 *
 *	@retval		TRUE		�|�[�^�u����
 *	@retval		FALSE		�ʏ�C���X�g�[����
 */
BOOL IsPortableMode(void)
{
	return FALSE;
}

/**
 *	�t�@�C����(�p�X��)����͂���
 *	GetFileNamePos() �� wchar_t��
 *
 *	@param[in]	PathName	�t�@�C�����A�t���p�X
 *	@param[out]	DirLen		�����̃X���b�V�����܂ރf�B���N�g���p�X��
 *							NULL�̂Ƃ��l��Ԃ��Ȃ�
 *	@param[out]	FNPos		�t�@�C�����ւ�index
 *							&PathName[FNPos] ���t�@�C����
 *							NULL�̂Ƃ��l��Ԃ��Ȃ�
 *	@retval		FALSE		PathName���s��
 */
BOOL GetFileNamePosW(const wchar_t *PathName, size_t *DirLen, size_t *FNPos)
{
	const wchar_t *Ptr;
	const wchar_t *DirPtr;
	const wchar_t *FNPtr;
	const wchar_t *PtrOld;

	if (DirLen != NULL) *DirLen = 0;
	if (FNPos != NULL) *FNPos = 0;

	if (PathName==NULL)
		return FALSE;

	if ((wcslen(PathName)>=2) && (PathName[1]==L':'))
		Ptr = &PathName[2];
	else
		Ptr = PathName;
	if (Ptr[0]=='\\' || Ptr[0]=='/')
		Ptr++;

	DirPtr = Ptr;
	FNPtr = Ptr;
	while (Ptr[0]!=0) {
		wchar_t b = Ptr[0];
		PtrOld = Ptr;
		Ptr++;
		switch (b) {
			case L':':
				return FALSE;
			case L'/':	/* FALLTHROUGH */
			case L'\\':
				DirPtr = PtrOld;
				FNPtr = Ptr;
				break;
		}
	}
	if (DirLen != NULL) *DirLen = DirPtr-PathName;
	if (FNPos != NULL) *FNPos = FNPtr-PathName;
	return TRUE;
}

/**
 *	ExtractFileName() �� wchar_t ��
 *	�t���p�X����t�@�C�������������o��
 *
 *	@return	�t�@�C��������(�s�v�ɂȂ�����free()����)
 */
wchar_t *ExtractFileNameW(const wchar_t *PathName)
{
	size_t i;
	if (!GetFileNamePosW(PathName, NULL, &i))
		return NULL;
	wchar_t *filename = _wcsdup(&PathName[i]);
	return filename;
}

/**
 *	ExtractDirName() �� wchar_t ��
 *
 *	@return	�f�B���N�g��������(�s�v�ɂȂ�����free()����)
 */
wchar_t *ExtractDirNameW(const wchar_t *PathName)
{
	size_t i;
	wchar_t *DirName = _wcsdup(PathName);
	if (!GetFileNamePosW(DirName, &i, NULL))
		return NULL;
	DirName[i] = 0;
	return DirName;
}

/*
 * Get Exe(exe,dll) directory
 *	ttermpro.exe, �v���O�C��������t�H���_
 *	ttypes.ExeDirW �Ɠ���
 *	���Ƃ� GetHomeDirW() ������
 *
 * @param[in]		hInst		WinMain()�� HINSTANCE �܂��� NULL
 * @return			ExeDir		�s�v�ɂȂ����� free() ���邱��
 */
wchar_t *GetExeDirW(HINSTANCE hInst)
{
	wchar_t *TempW;
	wchar_t *dir;
	DWORD error = hGetModuleFileNameW(hInst, &TempW);
	if (error != NO_ERROR) {
		// �p�X�̎擾�Ɏ��s�����B�v���I�Aabort() ����B
		abort();
	}
	dir = ExtractDirNameW(TempW);
	free(TempW);
	return dir;
}

/*
 * Get home directory
 *		�l�p�ݒ�t�@�C���t�H���_�擾
 *		ttypes.HomeDirW �Ɠ���
 *		TERATERM.INI �Ȃǂ������Ă���t�H���_
 *		ttermpro.exe ������t�H���_�� GetExeDirW() �Ŏ擾
 *		%APPDATA%\teraterm5 (%USERPROFILE%\AppData\Roaming\teraterm5)
 *
 * @param[in]		hInst		WinMain()�� HINSTANCE �܂��� NULL
 * @return			HomeDir		�s�v�ɂȂ����� free() ���邱��
 */
wchar_t *GetHomeDirW(HINSTANCE hInst)
{
	if (IsPortableMode()) {
		wchar_t *path;
		_SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path);
		wchar_t *ret = NULL;
		awcscats(&ret, path, L"\\teraterm5", NULL);
		free(path);
		return ret;
	}
	else {
		return GetExeDirW(hInst);
	}
}

/*
 * Get log directory
 *		���O�ۑ��t�H���_�擾
 *		ttypes.LogDirW �Ɠ���
 *		%LOCALAPPDATA%\teraterm5 (%USERPROFILE%\AppData\Local\teraterm5)
 *
 * @param[in]		hInst		WinMain()�� HINSTANCE �܂��� NULL
 * @return			LogDir		�s�v�ɂȂ����� free() ���邱��
 */
wchar_t* GetLogDirW(void)
{
	if (IsPortableMode()) {
		wchar_t *path;
		_SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);
		wchar_t *ret = NULL;
		awcscats(&ret, path, L"\\teraterm5", NULL);
		free(path);
		return ret;
	}
	else {
		wchar_t *path;
		_SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);
		return path;
	}
}
