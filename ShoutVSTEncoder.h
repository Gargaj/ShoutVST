#pragma once

#include <windows.h>
#include "shout/shout.h"

class ShoutVST;

class ShoutVSTEncoder
{
public:
  virtual bool Initialize(ShoutVST *) = NULL;
  virtual bool Close() = NULL;
  virtual bool Process( float **inputs, long sampleFrames ) = NULL;
};
