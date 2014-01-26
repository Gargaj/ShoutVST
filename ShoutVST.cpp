#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include "ShoutVST.h"

extern "C" BOOL pthread_win32_process_attach_np();

class CLock {
  CRITICAL_SECTION * cs;
public:
  CLock(CRITICAL_SECTION * c) : cs(c) {
    EnterCriticalSection(cs);
  }
  ~CLock() {
    LeaveCriticalSection(cs);
  }
};

ShoutVST::ShoutVST (audioMasterCallback audioMaster)
  : AudioEffectX (audioMaster, 1, 1),  // 1 program, 1 parameter only
  encMP3(this),
  encOGG(this)
{
  InitializeCriticalSection(&critsec);
  pEditor = NULL;
  Log("Constructing VST\r\n");
  setNumInputs(2);
  setNumOutputs(2);
  setUniqueID('SHOU');
  canMono ();
  canProcessReplacing();
  programsAreChunks(true);

  Log("shout_init()\r\n");
  shout_init();

  pShout = NULL;
  bStreamConnected = false;
  Log("Done...\r\n");

  bCanDoMP3 = encMP3.Preload();
  encSelected = &encMP3;

  pEditor = new ShoutVSTEditor(this);
  editor = pEditor;
  setEditor(editor);

  pthread_win32_process_attach_np();
}

bool ShoutVST::getEffectName(char* name)
{
  Log("getEffectName\r\n");
  strcpy (name, "ShoutVST");
  return true;
}

bool ShoutVST::getProductString(char* text)
{
  Log("getProductString\r\n");
  strcpy (text, "ShoutVST");
  return true;
}

bool ShoutVST::getVendorString (char* text)
{
  strcpy (text, "Gargaj / Conspiracy");
  return true;
}

void ShoutVST::setParameter (long index, float value)
{
  bool bShouldConnect = (value >= 0.5);

  {
    CLock lock(&critsec);

    if (bShouldConnect == bStreamConnected) return;

    switch (pEditor->nEncoder) {
      case SHOUT_FORMAT_OGG:
        {
          encSelected = &encOGG;
        } break;
      case SHOUT_FORMAT_MP3:
        {
          encSelected = &encMP3;
        } break;
    }
  
    Log("Parameter changed to %d\r\n",bShouldConnect ? 1 : 0);
    if (bShouldConnect) {
      Log("Initializing ICE...\r\n");
      if (!InitializeICECasting()) {
        StopICECasting();
        return;
      }
      Log("Initializing the encoder...\r\n");
      if (!encSelected->Initialize()) return;
    } else {
      Log("Stopping the encoder...\r\n");
      encSelected->Close();
      Log("Stopping ICE...\r\n");
      StopICECasting();
    }

    Log("setParameter succeeded!\r\n");

    bStreamConnected = bShouldConnect;

    pEditor->DisableAccordingly();
  }
 
}

float ShoutVST::getParameter (long index)
{
  return bStreamConnected ? 1.0f : 0.0f;
}

void ShoutVST::getParameterName (long index, char *label)
{
  strcpy (label, "Streaming");
}

void ShoutVST::getParameterDisplay (long index, char *text)
{
  strcpy(text,bStreamConnected ? "Yes" : "No");
}

void ShoutVST::getParameterLabel(long index, char *label)
{
  strcpy (label, "");
}

void ShoutVST::process (float **inputs, float **outputs, long sampleFrames)
{
  if(!inputs) return;
  if(!outputs) return;

  bool b = true;
  if (IsConnected())
  {
    CLock lock(&critsec);
    b = encSelected->Process(inputs,sampleFrames);
  }
  if (!b) {
    setParameter(0,0.0);
  }
  
  float *in1  =  inputs[0];
  float *in2  =  inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];

  while (--sampleFrames >= 0)
  {
    (*out1++) += (*in1++);
    (*out2++) += (*in2++);
  }
}

void ShoutVST::processReplacing(float **inputs, float **outputs, long sampleFrames)
{
  if(!inputs) return;
  if(!outputs) return;

  bool b = true;
  if (IsConnected())
  {
    CLock lock(&critsec);
    b = encSelected->Process(inputs,sampleFrames);
  }
  if (!b) {
    setParameter(0,0.0);
  }

  float *in1  =  inputs[0];
  float *in2  =  inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];

  while (--sampleFrames >= 0)
  {
    (*out1++) = (*in1++);
    (*out2++) = (*in2++);
  }
}

bool ShoutVST::InitializeICECasting()
{
  shout_t * shout = NULL;

  if (!(shout = shout_new())) {
    Log("Could not allocate shout_t\r\n");
    return false;
  }

  if (shout_set_host(shout, pEditor->szHostname) != SHOUTERR_SUCCESS) {
    Log("Error setting hostname: %s\r\n", shout_get_error(shout));
    return false;
  }

  if (shout_set_protocol(shout, pEditor->nProtocol) != SHOUTERR_SUCCESS) {
    Log("Error setting protocol: %s\r\n", shout_get_error(shout));
    return false;
  }

  if (shout_set_port(shout, pEditor->nPort) != SHOUTERR_SUCCESS) {
    Log("Error setting port: %s\r\n", shout_get_error(shout));
    return false;
  }

  if (shout_set_user(shout, pEditor->szUsername) != SHOUTERR_SUCCESS) {
    Log("Error setting user: %s\r\n", shout_get_error(shout));
    return false;
  }
  if (shout_set_password(shout, pEditor->szPassword) != SHOUTERR_SUCCESS) {
    Log("Error setting password: %s\r\n", shout_get_error(shout));
    return false;
  }
  if (shout_set_mount(shout, pEditor->szMountpoint) != SHOUTERR_SUCCESS) {
    Log("Error setting mount: %s\r\n", shout_get_error(shout));
    return false;
  }

  if (shout_set_format(shout, pEditor->nEncoder) != SHOUTERR_SUCCESS) {
    Log("Error setting encoder: %s\r\n", shout_get_error(shout));
    return false;
  }

  if (shout_open(shout) != SHOUTERR_SUCCESS) {
    Log("Error connecting: %s\r\n", shout_get_error(shout));
    return false;
  } 

  Log("Shout initialized!\r\n");
  pShout = shout;

  return true;
}

void ShoutVST::StopICECasting()
{
  Log("shout_close(%08X)\r\n",pShout);
  shout_close(pShout);

  pShout = NULL;
}

#define LOGLINELENGTH (40*1024)

void ShoutVST::Log(const char *fmt, ...)
{
  char text[LOGLINELENGTH];
  memset(text,0,LOGLINELENGTH);
  va_list ap;

  if (fmt == NULL)
    return;
  va_start(ap, fmt);
  _vsnprintf(text,LOGLINELENGTH-1, fmt, ap);
  va_end(ap);

  if (pEditor) pEditor->AppendLog(text);
#ifdef _DEBUG
  OutputDebugString(text);
#endif
}

ShoutVST::~ShoutVST()
{
  shout_shutdown(); 
}

bool ShoutVST::IsConnected()
{
  return bStreamConnected;
}

bool ShoutVST::SendDataToICE( unsigned char * pData, int nLen )
{
  shout_sync(pShout);
  if (shout_send(pShout,pData,nLen) != SHOUTERR_SUCCESS) 
  {
    Log("Error sending header: %s\r\n", shout_get_error(pShout));
    return false;
  }
  return true;
}

bool ShoutVST::CanDoMP3()
{
  return bCanDoMP3;
}

int ShoutVST::GetQuality()
{
  return pEditor->GetQuality();
}

void ShoutVST::AppendSerialize( char ** szString, char * szKey, char * szValue )
{
  char sz[1024];
  _snprintf(sz,1024,"%s=%s\n",szKey,szValue);

  int n = strlen(*szString) + strlen(sz) + 1;
  char * szNew = new char[n];
  strcpy(szNew,*szString);
  strcat(szNew,sz);
  delete[] *szString;
  *szString = szNew;
}

void ShoutVST::AppendSerialize( char ** szString, char * szKey, int nValue )
{
  char sz[1024];
  _snprintf(sz,1024,"%s=%d\n",szKey,nValue);

  int n = strlen(*szString) + strlen(sz) + 1;
  char * szNew = new char[n];
  strncpy(szNew,*szString,n);
  strncat(szNew,sz,n);
  delete[] *szString;
  *szString = szNew;
}

long ShoutVST::getChunk( void** data, bool isPreset /*= false */ )
{
  Log("getChunk(%d)\r\n",isPreset);

  char * sz = new char[1];
  *sz = 0;

  AppendSerialize( &sz,"hostname",pEditor->szHostname);
  AppendSerialize( &sz,"port",pEditor->nPort);
  AppendSerialize( &sz,"username",pEditor->szUsername);
  AppendSerialize( &sz,"password",pEditor->szPassword);
  AppendSerialize( &sz,"mountpoint",pEditor->szMountpoint);
  AppendSerialize( &sz,"encoder",pEditor->nEncoder);
  AppendSerialize( &sz,"protocol",pEditor->nProtocol);
  AppendSerialize( &sz,"quality",pEditor->GetQuality());

  *data = sz;

  return strlen(sz);
}

long ShoutVST::setChunk( void* data, long byteSize, bool isPreset /*= false */ )
{
  Log("setChunk(%d,%d)\r\n",byteSize,isPreset);
  
  char * sz = new char[ byteSize + 1 ];
  ZeroMemory(sz,byteSize + 1);
  CopyMemory(sz,data,byteSize);

  char * szEnd = NULL, * p = sz;
  while ( szEnd = strstr( p, "\n" ) )
  {
    char szLine[1024];
    ZeroMemory(szLine,1024);
    CopyMemory(szLine, p, szEnd - p);

    char * szEq = strstr(szLine,"=");
    char szKey[1024];
    ZeroMemory(szKey,1024);
    CopyMemory(szKey, szLine, szEq - szLine);

    if (strcmp(szKey,"hostname")==0) { ZeroMemory(pEditor->szHostname,MAX_PATH); sscanf(szLine,"hostname=%1023c",pEditor->szHostname); }
    if (strcmp(szKey,"port")==0) { sscanf(szLine,"port=%d",&pEditor->nPort); }
    if (strcmp(szKey,"username")==0) { ZeroMemory(pEditor->szUsername,MAX_PATH); sscanf(szLine,"username=%1023c",pEditor->szUsername); }
    if (strcmp(szKey,"password")==0) { ZeroMemory(pEditor->szPassword,MAX_PATH); sscanf(szLine,"password=%1023c",pEditor->szPassword); }
    if (strcmp(szKey,"mountpoint")==0) { ZeroMemory(pEditor->szMountpoint,MAX_PATH); sscanf(szLine,"mountpoint=%1023c",pEditor->szMountpoint); }
    if (strcmp(szKey,"encoder")==0) { sscanf(szLine,"encoder=%d",&pEditor->nEncoder); }
    if (strcmp(szKey,"protocol")==0) { sscanf(szLine,"protocol=%d",&pEditor->nProtocol); }
    if (strcmp(szKey,"quality")==0) { 
      int q = 0; 
      sscanf(szLine,"quality=%d",&q); 
      pEditor->SetQuality(q); 
    }

    p = szEnd + 1;
  }


  delete[] sz;

  return 0;
}

void ShoutVST::UpdateMetadata( char * sz )
{
  if (!pShout)
    return;

  CLock lock(&critsec);

  shout_metadata_t * meta = shout_metadata_new();
  if (!meta) return;

  if (shout_metadata_add( meta, "song", sz ) != SHOUTERR_SUCCESS) 
  {
    Log("Error adding metadata: %s\r\n", shout_get_error(pShout));
  }

  if (shout_set_metadata( pShout, meta ) != SHOUTERR_SUCCESS) 
  {
    Log("Error setting metadata: %s\r\n", shout_get_error(pShout));
  }

  shout_metadata_free(meta);

  Log("Set metadata to: %s\r\n", sz);

}
