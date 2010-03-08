#pragma once
#include "shoutvstencoder.h"
#include "blade/BladeMP3EncDLL.h"

class ShoutVSTEncoderMP3 :
  public ShoutVSTEncoder
{
public:
  ShoutVSTEncoderMP3( ShoutVST * p );
  bool Preload();
  bool Initialize();
  bool Close();
  bool Process( float **inputs, long sampleFrames );

protected:
  HMODULE hDLL;

  BEINITSTREAM beInitStream;
  BEENCODECHUNK beEncodeChunk;
  BEDEINITSTREAM beDeinitStream;
  BECLOSESTREAM beCloseStream;
  BEVERSION beVersion;
  BEWRITEVBRHEADER beWriteVBRHeader;
  BEWRITEINFOTAG beWriteInfoTag; 

  HBE_STREAM hbeStream;
  DWORD dwSamples;
  DWORD dwMP3Buffer;
  DWORD dwSamplesSoFar;
  BYTE * pMP3Buffer;
  SHORT * pWAVBuffer; 

  bool bInitialized;
};
