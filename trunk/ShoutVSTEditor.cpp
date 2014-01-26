#include <shout/shout.h>
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include "shoutvst.h"
#include "ShoutVSTEditor.h"
#include "resource.h"

ShoutVSTEditor::ShoutVSTEditor( AudioEffectX *effect ) : AEffEditor(effect)
{
  strncpy(szHostname,"localhost",MAX_PATH);
  nPort = 8000;
  strncpy(szUsername,"source",MAX_PATH);
  strncpy(szPassword,"hackme",MAX_PATH);
  strncpy(szMountpoint,"/shoutvst.mp3",MAX_PATH);
  nProtocol = SHOUT_PROTOCOL_HTTP;
  nEncoder = SHOUT_FORMAT_MP3;

  nQuality = 2;

  pVST = (ShoutVST*)effect;
  pVST->setEditor(this);
  if(!pVST->CanDoMP3()) {
    strncpy(szMountpoint,"/shoutvst.ogg",MAX_PATH);
    nEncoder = SHOUT_FORMAT_OGG;
  }

  szLog = new char[1];
  *szLog = 0;
}

ShoutVSTEditor::~ShoutVSTEditor()
{
  delete szLog;
}

extern HINSTANCE hInstance;

INT_PTR CALLBACK ShoutVSTDialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch ( uMsg ) {
    case WM_INITDIALOG:
      {
        SetWindowLong(hWnd,GWL_USERDATA,(LONG)lParam);
      } 
    default:
      {
        ShoutVSTEditor * w = (ShoutVSTEditor*)GetWindowLong(hWnd,GWL_USERDATA);
        if (w)
          return w->DialogProc(hWnd,uMsg,wParam,lParam);
        else
          return DefWindowProc(hWnd,uMsg,wParam,lParam);
      } break;
  }
}

INT_PTR ShoutVSTEditor::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch(uMsg) {
    case WM_INITDIALOG:
      {
        pVST->Log("[editor] passed hwnd: %08X\r\n",hWnd);
        hwndDialog = hWnd;

        SetWindowText(GetDlgItem(hWnd,IDC_HOSTNAME),szHostname);
        SetWindowText(GetDlgItem(hWnd,IDC_USERNAME),szUsername);
        SetWindowText(GetDlgItem(hWnd,IDC_PASSWORD),szPassword);
        SetWindowText(GetDlgItem(hWnd,IDC_MOUNTFILENAME),szMountpoint);
        SetWindowText(GetDlgItem(hWnd,IDC_HOSTNAME),szHostname);

        char s[30];
        sprintf(s,"%d",nPort);
        SetWindowText(GetDlgItem(hWnd,IDC_PORT),s);

        SendDlgItemMessage(hWnd,IDC_PROTOCOL,CB_ADDSTRING,NULL,(LPARAM)"HTTP");
        SendDlgItemMessage(hWnd,IDC_PROTOCOL,CB_ADDSTRING,NULL,(LPARAM)"XAudioCast");
        SendDlgItemMessage(hWnd,IDC_PROTOCOL,CB_ADDSTRING,NULL,(LPARAM)"ICY");
        SendDlgItemMessage(hWnd,IDC_PROTOCOL,CB_SETCURSEL,nProtocol,NULL);

        int n = 0;
        SendDlgItemMessage(hWnd,IDC_ENCODER,CB_ADDSTRING,NULL,(LPARAM)"OGG Vorbis");
        if (pVST->CanDoMP3()) {
          SendDlgItemMessage(hWnd,IDC_ENCODER,CB_ADDSTRING,NULL,(LPARAM)"Lame MP3");
          n++;
        }
        nEncoder = min(nEncoder,n);
        SendDlgItemMessage(hWnd,IDC_ENCODER,CB_SETCURSEL,nEncoder,NULL);
        //SendDlgItemMessage(hWnd,IDC_ENCODER,CB_SETMINVISIBLE,10,NULL);

        SendDlgItemMessage(hWnd,IDC_QUALITY,TBM_SETRANGE,TRUE,MAKELONG(0,10));
        SendDlgItemMessage(hWnd,IDC_QUALITY,TBM_SETPOS,TRUE,nQuality);

        ShowWindow(hWnd,SW_SHOW);

        DisableAccordingly();
        return TRUE;
      } break;
    case WM_DESTROY:
      {
        RefreshData();
      } break;
    case WM_COMMAND:
      {
        switch( LOWORD(wParam) ) {
          case IDC_ENCODER:
            {
              char sz[MAX_PATH];
              GetWindowText(GetDlgItem(hwndDialog,IDC_MOUNTFILENAME),sz,MAX_PATH);
              if (strchr(sz,'.'))
                *strrchr(sz,'.') = '\0';
              switch (SendDlgItemMessage(hwndDialog,IDC_ENCODER ,CB_GETCURSEL,NULL,NULL))
              {
                case SHOUT_FORMAT_OGG:
                  strcat(sz,".ogg");
                  break;
                case SHOUT_FORMAT_MP3:
                  strcat(sz,".mp3");
                  break;
              }
              SetWindowText(GetDlgItem(hwndDialog,IDC_MOUNTFILENAME),sz);
            } break;
          case IDC_QUALITY:
            {
              nQuality = SendDlgItemMessage(hwndDialog,IDC_QUALITY,TBM_GETPOS,NULL,NULL);
            } break;
          case IDC_CONNECT:
            {
              RefreshData();
              pVST->setParameter(0, 1.0f);
            } break;
          case IDC_DISCONNECT:
            {
              RefreshData();
              pVST->setParameter(0, 0.0f);
            } break;
          case IDC_SENDMETADATA:
            {
              char sz[2048];
              GetWindowText(GetDlgItem(hwndDialog,IDC_METADATA),sz,2047);
              RefreshData();
              pVST->UpdateMetadata(sz);
            } break;
        }
        DisableAccordingly();
      } break;
  }
  return FALSE;
}

long ShoutVSTEditor::open( void *ptr )
{
  pVST->Log("[editor] open\r\n");
  hwndParent = (HWND)ptr;

  hwndDialog = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_SHOUTVST_CONFIG),hwndParent,ShoutVSTDialogProc,(LPARAM)this);
  pVST->Log("[editor] hwnd: %08X\r\n",hwndDialog);
  return hwndDialog != NULL;
}

long ShoutVSTEditor::getRect(ERect **erect)
{
//  pVST->Log("[editor] getRect\r\n");
  RECT rc;
  GetWindowRect(hwndDialog,&rc);
  static ERect r = { 0, 0, 0, 0 };
  r.right = rc.right - rc.left;
  r.bottom = rc.bottom - rc.top;
  *erect = &r;
  return true;
}

void ShoutVSTEditor::AppendLog( char * sz )
{
  int n = strlen(szLog) + strlen(sz) + 10;

  char * p = new char[n];
  memset(p,0,n);

  strcat(p,szLog);
  strcat(p,sz);

  delete szLog;
  szLog = p;

  SetWindowText(GetDlgItem(hwndDialog,IDC_LOG),szLog);

  int nLines = 1;
  for(int n=0; szLog[n]; n++) if (szLog[n]=='\n') nLines++;
  SendMessage(GetDlgItem(hwndDialog,IDC_LOG), EM_LINESCROLL, 0, nLines);
}

void ShoutVSTEditor::RefreshData()
{
  GetWindowText(GetDlgItem(hwndDialog,IDC_HOSTNAME),szHostname,MAX_PATH);
  GetWindowText(GetDlgItem(hwndDialog,IDC_USERNAME),szUsername,MAX_PATH);
  GetWindowText(GetDlgItem(hwndDialog,IDC_PASSWORD),szPassword,MAX_PATH);
  GetWindowText(GetDlgItem(hwndDialog,IDC_MOUNTFILENAME),szMountpoint,MAX_PATH);
  GetWindowText(GetDlgItem(hwndDialog,IDC_HOSTNAME),szHostname,MAX_PATH);

  char s[30];
  GetWindowText(GetDlgItem(hwndDialog,IDC_PORT),s,30);
  sscanf_s(s,"%d",&nPort);

  nProtocol = SendDlgItemMessage(hwndDialog,IDC_PROTOCOL,CB_GETCURSEL,NULL,NULL);
  nEncoder  = SendDlgItemMessage(hwndDialog,IDC_ENCODER ,CB_GETCURSEL,NULL,NULL);
  nQuality  = SendDlgItemMessage(hwndDialog,IDC_QUALITY ,TBM_GETPOS,NULL,NULL);
}

void ShoutVSTEditor::DisableAccordingly()
{
//  pVST->Log("[editor] disabling\r\n");
  EnableWindow(GetDlgItem(hwndDialog,IDC_HOSTNAME), !pVST->IsConnected());
  EnableWindow(GetDlgItem(hwndDialog,IDC_PORT), !pVST->IsConnected());
  EnableWindow(GetDlgItem(hwndDialog,IDC_USERNAME), !pVST->IsConnected());
  EnableWindow(GetDlgItem(hwndDialog,IDC_PASSWORD), !pVST->IsConnected());
  EnableWindow(GetDlgItem(hwndDialog,IDC_PROTOCOL), !pVST->IsConnected());
  EnableWindow(GetDlgItem(hwndDialog,IDC_MOUNTFILENAME), !pVST->IsConnected());
  EnableWindow(GetDlgItem(hwndDialog,IDC_ENCODER), !pVST->IsConnected());
  EnableWindow(GetDlgItem(hwndDialog,IDC_QUALITY), !pVST->IsConnected());

  EnableWindow(GetDlgItem(hwndDialog,IDC_CONNECT)     ,!pVST->IsConnected());
  EnableWindow(GetDlgItem(hwndDialog,IDC_DISCONNECT)  , pVST->IsConnected());
  EnableWindow(GetDlgItem(hwndDialog,IDC_SENDMETADATA), pVST->IsConnected());
  EnableWindow(GetDlgItem(hwndDialog,IDC_METADATA),     pVST->IsConnected());
}

int ShoutVSTEditor::GetQuality()
{
  return nQuality;//SendDlgItemMessage(hwndDialog,IDC_QUALITY,TBM_GETPOS,NULL,NULL);
}

void ShoutVSTEditor::SetQuality( int q )
{
  nQuality = q;
  SendDlgItemMessage(hwndDialog,IDC_QUALITY,TBM_SETPOS,TRUE,q);
}