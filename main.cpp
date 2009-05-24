#include <windows.h>
#include "ShoutVST.h"

bool oome = false;

#define VST_EXPORT(rt) extern "C" __declspec(dllexport) rt __cdecl

VST_EXPORT(int) main(audioMasterCallback audioMaster)
{
  //ShoutVST::PrintF("[main] Get VST Version \n");
  if (!audioMaster (0, audioMasterVersion, 0, 0, 0, 0))
    return 0;  // old version

  //ShoutVST::PrintF("[main] new ShoutVST\n");
  ShoutVST * effect = new ShoutVST(audioMaster);
  if (!effect)
    return 0;

  //ShoutVST::PrintF("[main] Check if no problem\n");
  if (oome)
  {
    delete effect;
    return 0;
  }
  //ShoutVST::PrintF("[main] return\n");
  return (int)effect->getAeffect();
}

HINSTANCE hInstance;

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
  //ShoutVST::PrintF("[main] DllMain - dwReason: %d\n",dwReason);
  hInstance = hInst;
  return 1;
}
