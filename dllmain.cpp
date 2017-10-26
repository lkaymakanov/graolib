// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <WinCon.h>
#include <stdio.h>
#include "GraoImpl.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		initWin1251TounicodeMap();
		break;

	case DLL_THREAD_ATTACH:

	case DLL_THREAD_DETACH:


	case DLL_PROCESS_DETACH:
		//printf("Dll main called - reason for call is %d ", ul_reason_for_call);
		break;
	}
	return TRUE;
}

