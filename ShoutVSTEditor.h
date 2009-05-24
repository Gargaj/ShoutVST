#pragma once
#include <windows.h>
#include "common/aeffeditor.hpp"
#include "common/audioeffectx.h"

class ShoutVST;

class ShoutVSTEditor :
  public AEffEditor
{
public:
  ShoutVSTEditor(AudioEffectX *effect);
  virtual ~ShoutVSTEditor();
  virtual long open(void *ptr);
  virtual long getRect(ERect **erect);

  INT_PTR DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  void AppendLog(char *);

  char szHostname[MAX_PATH];
  int nPort;
  char szUsername[MAX_PATH];
  char szPassword[MAX_PATH];
  char szMountpoint[MAX_PATH];
  int nProtocol;

  int nEncoder;

  int GetQuality();

  void RefreshData();
  void DisableAccordingly();

protected:
  ShoutVST * pVST;
  HWND hwndParent;
  HWND hwndDialog;

  char * szLog;
};
