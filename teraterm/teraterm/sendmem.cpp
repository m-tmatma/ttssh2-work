/*
 * (C) 2019 TeraTerm Project
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

#include <windows.h>
#include <stdlib.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include "tttypes.h"
#include "ttcommon.h"
#include "ftdlg_lite.h"

#include "sendmem.h"

typedef struct SendMemTag {
	const BYTE *send_ptr;  // ���M�f�[�^�ւ̃|�C���^
	size_t send_len;	   // ���M�f�[�^�T�C�Y
	SendMemType type;
	BOOL local_echo_enable;
	BOOL send_enable;
	DWORD delay_per_line;  // (ms)
	DWORD delay_per_char;
	HWND hWnd;	 // �^�C�}�[���󂯂�window
	int timer_id;  // �^�C�}�[ID
	char *UILanguageFile;
	wchar_t *dialog_caption;
	wchar_t *filename;
	//
	size_t send_left;
	size_t send_index;
	BOOL waited;
	DWORD last_send_tick;
	//
	CFileTransLiteDlg *dlg;
	class SendMemDlgObserver *dlg_observer;
	HINSTANCE hInst_;
	HWND hWndParent_;
	//
	PComVar cv_;
	BOOL pause;
	const BYTE *send_out_retry_ptr;
	size_t send_out_retry_len;
	const BYTE *send_echo_retry_ptr;
	size_t send_echo_retry_len;
} SendMem;

typedef SendMem sendmem_work_t;

extern "C" IdTalk TalkStatus;
extern "C" HWND HVTWin;

static sendmem_work_t *sendmem_work;

class SendMemDlgObserver : public CFileTransLiteDlg::Observer {
public:
	SendMemDlgObserver(HWND hWndParent, void (*OnClose)(), void (*OnPause)(BOOL paused)) {
		hWndParent_ = hWndParent;
		OnClose_ = OnClose;
		OnPause_ = OnPause;
	}
private:
	void OnClose()
	{
		OnClose_();
	};
	void OnPause(BOOL paused)
	{
		OnPause_(paused);
	};
	void OnHelp()
	{
		MessageBoxA(hWndParent_, "no help topic", "Tera Term", MB_OK);
	}
	HWND hWndParent_;
	void (*OnClose_)();
	void (*OnPause_)(BOOL paused);
};

static void EndPaste()
{
	sendmem_work_t *p = sendmem_work;
	p->send_ptr = NULL;

	TalkStatus = IdTalkKeyb;
	if (p->dlg != NULL) {
		p->dlg->Destroy();
		delete p->dlg;
		delete p->dlg_observer;
	}
	free(p->UILanguageFile);
	free(p->dialog_caption);
	free(p->filename);
	free(p);

	sendmem_work = NULL;

	// ����ł���悤�ɂ���
#if 0
	EnableWindow(HVTWin, TRUE);
#endif
}

static void OnClose()
{
	EndPaste();
}

static void OnPause(BOOL paused)
{
	sendmem_work_t *p = sendmem_work;
	p->pause = paused;
	if (!paused) {
		// �|�[�Y������, �^�C�}�[�C�x���g�ōđ��̃g���K������
		SetTimer(p->hWnd, p->timer_id, 0, NULL);
	}
}

static void SendMemStart_i(SendMem *sm)
{
	sendmem_work = sm;
	sendmem_work_t *p = sm;

	p->send_left = p->send_len;
	p->send_index = 0;
	p->send_out_retry_ptr = NULL;
	p->send_echo_retry_ptr = NULL;

	p->waited = FALSE;
	p->pause = FALSE;

	if (p->hWndParent_ != NULL) {
		// �_�C�A���O�𐶐�����
		p->dlg = new CFileTransLiteDlg();
		p->dlg->Create(NULL, NULL, sm->UILanguageFile);
		if (p->dialog_caption != NULL) {
			p->dlg->SetCaption(p->dialog_caption);
		}
		if (p->filename != NULL) {
			p->dlg->SetFilename(p->filename);
		}
		p->dlg_observer = new SendMemDlgObserver(p->hWndParent_, OnClose, OnPause);
		p->dlg->SetObserver(p->dlg_observer);
	}

	TalkStatus = IdTalkSendMem;

	// ���M�J�n���ɃE�B���h�E�𑀍�ł��Ȃ��悤�ɂ���
#if 0
	EnableWindow(HVTWin, FALSE);
#endif
}

/**
 * ���M
 */
void SendMemContinuously(void)
{
	sendmem_work_t *p = sendmem_work;

	if (p->send_ptr == NULL) {
		EndPaste();
		return;
	}

	if (p->pause) {
		return;
	}

	if (p->waited) {
		const DWORD delay = p->delay_per_line > 0 ? p->delay_per_line : p->delay_per_char;
		if (GetTickCount() - p->last_send_tick < delay) {
			// �E�G�C�g����
			return;
		}
	}

	// ���M�o�b�t�@(echo)�ł��Ȃ����������đ�
	int r;
	size_t sended_size = 0;
	if (p->send_out_retry_ptr != NULL) {
		if (p->type == SendMemTypeBinary) {
			r = CommBinaryBuffOut(p->cv_, (PCHAR)p->send_out_retry_ptr, (int)p->send_out_retry_len);
		}
		else {
			// text (not binary)
			r = CommTextOutW(p->cv_, (wchar_t *)p->send_out_retry_ptr, (int)(p->send_out_retry_len / sizeof(wchar_t)));
			r *= sizeof(wchar_t);
		}
		sended_size = r;
		p->send_out_retry_len -= r;
		if (p->send_out_retry_len == 0) {
			p->send_out_retry_ptr = NULL;
		}
		else {
			p->send_out_retry_ptr += r;
		}
	}
	if (p->send_echo_retry_ptr != NULL) {
		size_t echo_size = sended_size < p->send_echo_retry_len ? sended_size : p->send_echo_retry_len;
		if (p->type == SendMemTypeBinary) {
			r = CommTextEcho(p->cv_, (PCHAR)p->send_echo_retry_ptr, (int)echo_size);
		}
		else {
			r = CommTextEchoW(p->cv_, (wchar_t *)p->send_echo_retry_ptr, (int)(echo_size / sizeof(wchar_t)));
			r *= sizeof(wchar_t);
		}
		p->send_echo_retry_len -= r;
		if (p->send_echo_retry_len == 0) {
			p->send_echo_retry_ptr = NULL;
		}
		else {
			p->send_echo_retry_ptr += r;
		}
	}
	if (p->send_out_retry_ptr != NULL || p->send_echo_retry_ptr != NULL) {
		// �܂��S�����M�ł��Ă��Ȃ�
		return;
	}

	// ���M
	for (;;) {
		// �I�[?
		if (p->send_left == 0) {
			// �I��
			EndPaste();
			return;
		}

		// ���M��
		BOOL need_delay = FALSE;
		size_t send_len;
		if (p->delay_per_char > 0) {
			// 1�L�����N�^���M
			need_delay = TRUE;
			if (p->type == SendMemTypeBinary) {
				send_len = 1;
			}
			else {
				send_len = sizeof(wchar_t);
			}
		}
		else if (p->delay_per_line > 0) {
			// 1���C�����M
			need_delay = TRUE;

			// 1�s���o��(���s�R�[�h�� 0x0a �ɐ��K������Ă���)
			const wchar_t *line_top = (wchar_t *)&p->send_ptr[p->send_index];
			const wchar_t *line_end = wcschr(line_top, 0x0a);
			if (line_end != NULL) {
				// 0x0a �܂ő��M
				send_len = ((line_end - line_top) + 1) * sizeof(wchar_t);
			}
			else {
				// ���s�����炸�A�Ō�܂ő��M
				send_len = p->send_left;
			}
		}
		else {
			// �S�͑��M
			send_len = p->send_left;
		}

		// ���M����
		const BYTE *send_ptr = (BYTE *)&p->send_ptr[p->send_index];
		p->send_index += send_len;
		p->send_left -= send_len;
		size_t sended_len;
		if (p->type == SendMemTypeBinary) {
			sended_len = CommBinaryBuffOut(p->cv_, (PCHAR)send_ptr, (int)send_len);
		}
		else {
			sended_len = CommTextOutW(p->cv_, (wchar_t *)send_ptr, (int)(send_len / sizeof(wchar_t)));
			sended_len *= sizeof(wchar_t);
		}
		if ((sended_len != 0) && (p->local_echo_enable)) {
			// ���M�ł�����echo����
			size_t echo_len = sended_len;
			size_t echoed_len;
			if (p->type == SendMemTypeBinary) {
				echoed_len = CommTextEcho(p->cv_, (PCHAR)send_ptr, (int)echo_len);
			}
			else {
				echoed_len = CommTextEchoW(p->cv_, (wchar_t *)send_ptr, (int)(echo_len / sizeof(wchar_t)));
				sended_len *= sizeof(wchar_t);
			}
			if (echoed_len != echo_len) {
				p->send_out_retry_ptr = send_ptr + echoed_len;
				p->send_out_retry_len = echo_len - echoed_len;
			}
		}
		if (sended_len < send_len) {
			// ���ׂđ��M�ł��Ȃ�����
			p->send_out_retry_ptr = send_ptr + sended_len;
			p->send_out_retry_len = send_len - sended_len;
		}
		if (p->send_out_retry_ptr != NULL || p->send_echo_retry_ptr != NULL) {
			// �o�͂ł��Ȃ��Ȃ���(�o�̓o�b�t�@�������ς�?)
			return;
		}

		// �_�C�A���O�X�V
		if (p->dlg != NULL) {
			p->dlg->RefreshNum(p->send_index, p->send_len);
		}

		if (need_delay) {
			// wait�ɓ���
			p->waited = TRUE;
			p->last_send_tick = GetTickCount();
			// �^�C�}�[��idle�𓮍삳���邽�߂Ɏg�p���Ă���
			const DWORD delay = p->delay_per_line > 0 ? p->delay_per_line : p->delay_per_char;
			SetTimer(p->hWnd, p->timer_id, delay, NULL);
			break;
		}
	}
}

/*
 *	���s�R�[�h��LF(0x0a)�����ɂ���
 *
 *	@param [in]	*src_		������ւ̃|�C���^
 *	@param [in] *len		���͕�����(0�̂Ƃ������ŕ����񒷂𑪂�)
 *	@param [out] *len		�o�͕�����
 *	@return					�ϊ��㕶����(malloc���ꂽ�̈�)
 */
static wchar_t *NormalizeLineBreak(const wchar_t *src, size_t *len)
{
	size_t src_len = *len;
	if (src_len == 0) {
		return NULL;
	}
	wchar_t *dest_top = (wchar_t *)malloc(sizeof(wchar_t) * src_len);
	if (dest_top == NULL) {
		return NULL;
	}

	// LF(0x0a),CR(0x0d)�����邩���ׂ�
	int cr_count = 0;
	int lf_count = 0;
	const wchar_t *p = src;
	for (size_t i = 0; i < src_len; i++) {
		wchar_t c = *p++;
		if (c == CR) {	// 0x0d
			cr_count++;
		} else if (c == LF) {	// 0x0d
			lf_count++;
		}
	}

	wchar_t *dest = dest_top;
	if (lf_count == 0 && cr_count != 0) {
		// LF�Ȃ��ACR�̂�
		// CR��LF�ɕϊ�����
		for (size_t i = 0; i < src_len; i++) {
			wchar_t c = *src++;
			*dest++ = (c == CR) ? LF : c;
		}
	} else if (lf_count != 0 && cr_count != 0) {
		// CR�ALF�Ƃ�����
		// CR���̂Ă�
		for (size_t i = 0; i < src_len; i++) {
			wchar_t c = *src++;
			if (c == CR) {
				continue;
			}
			*dest++ = c;
		}
	} else {
		// CR�̂� or ���s���S���Ȃ�
		// �ϊ��s�v
		memcpy(dest , src, sizeof(wchar_t) * src_len);
		dest += src_len;
	}

	*len = dest - dest_top;
	return dest_top;
}

/**
 *	�������ɂ���f�[�^�𑗐M����
 *	�f�[�^�͑��M�I�����free()�����
 *
 *	@param	ptr		�f�[�^�փ|�C���^(malloc()���ꂽ�̈�)
 *					���M��(���f��)�A�����I��free()�����
 *	@param	len		�f�[�^��(byte)
 *					������(wchar_t)�̏ꍇ��byte��
 *	@param	type	������(wchar_t,LF or CRLF or �o�C�i��)
 */
SendMem *SendMemInit(void *ptr, size_t len, SendMemType type)
{
	SendMem *p = (SendMem *)calloc(sizeof(*p), 1);
	if (p == NULL) {
		return NULL;
	}
	if (type == SendMemTypeTextCRLF || type == SendMemTypeTextLF) {
		// ���s�R�[�h�𒲐����Ă���
		size_t new_len = len / sizeof(wchar_t);
		p->send_ptr = (BYTE *)NormalizeLineBreak((wchar_t *)ptr, &new_len);
		p->send_len = new_len * sizeof(wchar_t);
		free(ptr);
	} else {
		p->send_ptr = (BYTE *)ptr;
		p->send_len = len;
	}
	if (p->send_ptr == NULL) {
		return NULL;
	}
	p->type = type;
	p->local_echo_enable = FALSE;
	p->delay_per_char = 0;  // (ms)
	p->delay_per_line = 0;  // (ms)
	p->cv_ = NULL;
	p->hWnd = HVTWin;		// delay���Ɏg�p����^�C�}�[�p
	p->timer_id = IdPasteDelayTimer;
	p->hWndParent_ = NULL;
	return p;
}

void SendMemInitEcho(SendMem *sm, BOOL echo)
{
	sm->local_echo_enable = echo;
}

void SendMemInitDelay(SendMem* sm, DWORD per_line, DWORD per_char)
{
	sm->delay_per_char = per_char;	// (ms)
	sm->delay_per_line = per_line;
}

// �Z�b�g����ƃ_�C�A���O���o��
void SendMemInitDialog(SendMem *sm, HINSTANCE hInstance, HWND hWndParent, const char *UILanguageFile)
{
	sm->hInst_ = hInstance;
	sm->hWndParent_ = hWndParent;
	sm->UILanguageFile = _strdup(UILanguageFile);
}

void SendMemInitDialogCaption(SendMem *sm, const wchar_t *caption)
{
	if (sm->dialog_caption != NULL)
		free(sm->dialog_caption);
	sm->dialog_caption = _wcsdup(caption);
}

void SendMemInitDialogFilename(SendMem *sm, const wchar_t *filename)
{
	if (sm->filename != NULL)
		free(sm->filename);
	sm->filename = _wcsdup(filename);
}

extern "C" TComVar cv;
BOOL SendMemStart(SendMem *sm)
{
	// ���M���`�F�b�N
	if (sendmem_work != NULL) {
		return FALSE;
	}

	sm->cv_ = &cv;		// TODO �Ȃ�������
	SendMemStart_i(sm);
	return TRUE;
}

void SendMemFinish(SendMem *sm)
{
	free(sm->UILanguageFile);
	free(sm);
}
