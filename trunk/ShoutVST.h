#pragma once

#include "common/audioeffectx.h"
#include "shout/shout.h"
#include "ShoutVSTEditor.h"

#include "ShoutVSTEncoderOGG.h"
#include "ShoutVSTEncoderMP3.h"

//-------------------------------------------------------------------------------------------------------
class ShoutVST : public AudioEffectX
{
public:
  ShoutVST(audioMasterCallback audioMaster);
  ~ShoutVST();

  // Processes
  virtual void process(float **inputs, float **outputs, long sampleFrames);
  virtual void ShoutVST::processReplacing(float **inputs, float **outputs, long sampleFrames);

  virtual bool getEffectName(char* name);
  virtual bool getVendorString(char* text);
  virtual bool getProductString(char* text);

  virtual void setParameter (long index, float value);
  virtual float getParameter (long index);
  virtual void getParameterLabel (long index, char *label);
  virtual void getParameterDisplay (long index, char *text);
  virtual void getParameterName (long index, char *text);

  virtual VstPlugCategory getPlugCategory () { return kPlugCategEffect; }

  void Log(const char *fmt, ...);
  //static void PrintF(const char *fmt, ...);

  bool IsConnected();
  bool SendDataToICE(unsigned char *, int);
  bool CanDoMP3();
  int GetQuality(); // returns 0 - 10

  CRITICAL_SECTION critsec;

protected:
  bool InitializeICECasting();
  void ProcessICECasting(float **inputs, long sampleFrames);
  void StopICECasting();

  virtual long getChunk( void** data, bool isPreset = false );

  virtual long setChunk( void* data, long byteSize, bool isPreset = false );

  void AppendSerialize( char ** szString, char * szKey, char * szValue );
  void AppendSerialize( char ** szString, char * szKey, int szValue );
  
  bool bCanDoMP3;

  ShoutVSTEncoder * encSelected;
  ShoutVSTEncoderOGG encOGG;
  ShoutVSTEncoderMP3 encMP3;

  ShoutVSTEditor * pEditor;

private:
  shout_t * pShout;
  bool bStreamConnected;
};
