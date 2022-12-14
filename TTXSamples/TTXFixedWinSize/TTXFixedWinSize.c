#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
// #include "ttlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define WIDTH 80
#define HEIGHT 24

#define ORDER 5990

static HANDLE hInst; /* Instance handle of TTX*.DLL */

typedef struct {
  PTTSet ts;
  PComVar cv;
  BOOL sizeModify;
  PSetupTerminal origSetupTerminalDlg;
  PReadIniFile origReadIniFile;
} TInstVar;

static TInstVar *pvar;
static TInstVar InstVar;

static void PASCAL TTXInit(PTTSet ts, PComVar cv) {
    pvar->ts = ts;
    pvar->cv = cv;

    pvar->sizeModify = FALSE;
    pvar->origSetupTerminalDlg = NULL;
    pvar->origReadIniFile = NULL;
}

static BOOL PASCAL FixedSizeSetupTerminalDlg(HWND parent, PTTSet ts) {
    BOOL ret;
    if (pvar->sizeModify) {
	pvar->ts->TerminalWidth = WIDTH;
	pvar->ts->TerminalHeight = HEIGHT;
	pvar->sizeModify = FALSE;
	ret = TRUE;
    }
    else {
	ret = (pvar->origSetupTerminalDlg)(parent, ts);
	if (ret) {
	    pvar->ts->TerminalWidth = WIDTH;
	    pvar->ts->TerminalHeight = HEIGHT;
	}
    }

    return ret;
}

static void PASCAL TTXGetUIHooks(TTXUIHooks *hooks) {
    pvar->origSetupTerminalDlg = *hooks->SetupTerminal;
    *hooks->SetupTerminal = FixedSizeSetupTerminalDlg;
}

static void PASCAL FixedSizeReadIniFile(const wchar_t *fn, PTTSet ts) {
    (pvar->origReadIniFile)(fn, ts);
    ts->TerminalWidth = WIDTH;
    ts->TerminalHeight = HEIGHT;
}

static void PASCAL TTXGetSetupHooks(TTXSetupHooks *hooks) {
    pvar->origReadIniFile = *hooks->ReadIniFile;
    *hooks->ReadIniFile = FixedSizeReadIniFile;
}

static void PASCAL TTXSetWinSize(int rows, int cols) {
    if (rows != HEIGHT || cols != WIDTH) {
	pvar->sizeModify = TRUE;
	// Call Setup-Terminal dialog
	SendMessage(pvar->cv->HWin, WM_COMMAND, MAKELONG(50310, 0), 0);
    }
}

static TTXExports Exports = {
  sizeof(TTXExports),
  ORDER,

  TTXInit,
  TTXGetUIHooks,
  TTXGetSetupHooks,
  NULL, // TTXOpenTCP,
  NULL, // TTXCloseTCP,
  TTXSetWinSize,
  NULL, // TTXModifyMenu,
  NULL, // TTXModifyPopupMenu,
  NULL, // TTXProcessCommand,
  NULL, // TTXEnd
};

BOOL __declspec(dllexport) PASCAL TTXBind(WORD Version, TTXExports *exports) {
  int size = sizeof(Exports) - sizeof(exports->size);

  if (size > exports->size) {
    size = exports->size;
  }
  memcpy((char *)exports + sizeof(exports->size),
    (char *)&Exports + sizeof(exports->size),
    size);
  return TRUE;
}

BOOL WINAPI DllMain(HANDLE hInstance,
		    ULONG ul_reason_for_call,
		    LPVOID lpReserved)
{
  switch( ul_reason_for_call ) {
    case DLL_THREAD_ATTACH:
      /* do thread initialization */
      break;
    case DLL_THREAD_DETACH:
      /* do thread cleanup */
      break;
    case DLL_PROCESS_ATTACH:
      /* do process initialization */
      hInst = hInstance;
      pvar = &InstVar;
      break;
    case DLL_PROCESS_DETACH:
      /* do process cleanup */
      break;
  }
  return TRUE;
}
