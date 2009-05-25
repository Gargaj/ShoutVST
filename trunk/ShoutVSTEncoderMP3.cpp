#include <stdio.h>
#include "ShoutVST.h"
#include "ShoutVSTEncoderMP3.h"

// note to self
#define STEREO 2

extern HINSTANCE hInstance;
bool ShoutVSTEncoderMP3::Preload()
{
  hDLL = LoadLibrary("lame_enc.dll");

  if (!hDLL) {
    char s[MAX_PATH];
    GetModuleFileName(hInstance,s,MAX_PATH);
    if (strrchr(s,'\\')) {
      *strrchr(s,'\\') = 0;
    }
    strcat(s,"\\lame_enc.dll");

    hDLL = LoadLibrary(s);

    if (!hDLL)
      return false;
  }

  beInitStream = (BEINITSTREAM) GetProcAddress(hDLL, TEXT_BEINITSTREAM);
  beEncodeChunk = (BEENCODECHUNK) GetProcAddress(hDLL, TEXT_BEENCODECHUNK);
  beDeinitStream  = (BEDEINITSTREAM) GetProcAddress(hDLL, TEXT_BEDEINITSTREAM);
  beCloseStream = (BECLOSESTREAM) GetProcAddress(hDLL, TEXT_BECLOSESTREAM);
  beVersion   = (BEVERSION) GetProcAddress(hDLL, TEXT_BEVERSION);
  beWriteVBRHeader= (BEWRITEVBRHEADER) GetProcAddress(hDLL,TEXT_BEWRITEVBRHEADER);
  beWriteInfoTag  = (BEWRITEINFOTAG) GetProcAddress(hDLL,TEXT_BEWRITEINFOTAG); 

  if(!beInitStream || !beEncodeChunk || !beDeinitStream || !beCloseStream || !beVersion || !beWriteVBRHeader) 
    return false;

  return true;
}

bool ShoutVSTEncoderMP3::Initialize( ShoutVST * p )
{
  bInitialized = false;

  pVST = p;
  hbeStream = NULL;

  BE_VERSION v;
  beVersion( &v ); 

  pVST->Log("[mp3] lame_enc.dll version %u.%02u\r\n",v.byDLLMajorVersion, v.byDLLMinorVersion);

  BE_CONFIG cfg;
  memset(&cfg,0,sizeof(BE_CONFIG));

  cfg.dwConfig = BE_CONFIG_LAME;

  cfg.format.LHV1.dwStructVersion = 1;
  cfg.format.LHV1.dwStructSize    = sizeof(BE_CONFIG);    
  cfg.format.LHV1.dwSampleRate    = (long)pVST->updateSampleRate();
  cfg.format.LHV1.nMode           = BE_MP3_MODE_JSTEREO;
  cfg.format.LHV1.dwMaxBitrate    = 256;
  cfg.format.LHV1.bWriteVBRHeader = TRUE;
  cfg.format.LHV1.bEnableVBR      = TRUE;
  cfg.format.LHV1.nVBRQuality     = min(10 - pVST->GetQuality(), 9);

/*
  cfg.format.LHV1.nPreset         = LQP_R3MIX;
  cfg.format.LHV1.dwMpegVersion   = MPEG1;
  cfg.format.LHV1.bOriginal       = TRUE;
*/

  if(beInitStream(&cfg, &dwSamples, &dwMP3Buffer, &hbeStream) != BE_ERR_SUCCESSFUL)
  {
    pVST->Log("[mp3] Error opening encoding stream\r\n");
    return false;
  } 
  pMP3Buffer = new BYTE[dwMP3Buffer];
  pWAVBuffer = new SHORT[dwSamples]; 
  dwSamplesSoFar = 0;
  dwSamples /= STEREO;

  bInitialized = true;

  return true;
}

bool ShoutVSTEncoderMP3::Close()
{
  bInitialized = false;
  DWORD dwWrite = 0;
  if(beDeinitStream(hbeStream, pMP3Buffer, &dwWrite) != BE_ERR_SUCCESSFUL)
  {
    beCloseStream(hbeStream);
    pVST->Log("[mp3] Error in beDeinitStream\r\n");
    return false;
  }
  pVST->SendDataToICE(pMP3Buffer,dwWrite);
  delete[] pWAVBuffer;
  delete[] pMP3Buffer;

  return true;
}

bool ShoutVSTEncoderMP3::Process( float **inputs, long sampleFrames )
{
  if (!bInitialized) return false;

  int nSamplesToProcess = sampleFrames;
  int nInputPointer = 0;
  while (nSamplesToProcess > 0) {

    int nSampleRoom = dwSamples - dwSamplesSoFar;
    int nSamplesProcessed = min(nSampleRoom,nSamplesToProcess);

    SHORT * p = pWAVBuffer + dwSamplesSoFar * STEREO;
    for (int i=0; i < nSamplesProcessed; i++) {
      *(p++) = (SHORT)(min(1.0f,max(-1.0f,inputs[0][nInputPointer])) * 32767.0f);
      *(p++) = (SHORT)(min(1.0f,max(-1.0f,inputs[1][nInputPointer])) * 32767.0f);
      nInputPointer++;
    }
    dwSamplesSoFar += nSamplesProcessed;

    if (dwSamplesSoFar == dwSamples) {
      DWORD dwWrite = 0;
      if(beEncodeChunk(hbeStream, dwSamples * STEREO, pWAVBuffer, pMP3Buffer, &dwWrite) != BE_ERR_SUCCESSFUL)
      {
        beCloseStream(hbeStream);
        pVST->Log("[mp3] Error in beDeinitStream\r\n");
        return false;
      }
      
/*
      DWORD dw = 0;
      HANDLE hLogFile = CreateFile( "dump.raw", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, NULL, NULL);
      SetFilePointer(hLogFile,0,NULL,FILE_END);
      WriteFile(hLogFile,pWAVBuffer,dwSamples * STEREO * sizeof(SHORT),&dw,NULL);
      CloseHandle(hLogFile);
*/

      //pVST->Log("[mp3] Send data: %d\r\n",dwWrite);
      if (!pVST->SendDataToICE(pMP3Buffer,dwWrite)) return false;
      dwSamplesSoFar = 0;
    }

    nSamplesToProcess -= nSamplesProcessed;

  }

  return true;
}