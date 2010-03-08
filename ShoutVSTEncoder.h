#pragma once

#include <windows.h>
#include "shout/shout.h"

class ShoutVST;

class ShoutVSTEncoder
{
public:
  ShoutVSTEncoder(ShoutVST *);
  virtual bool Initialize() = NULL;
  virtual bool Close() = NULL;
  virtual bool Process( float **inputs, long sampleFrames ) = NULL;

protected:
  ShoutVST * pVST;
};
