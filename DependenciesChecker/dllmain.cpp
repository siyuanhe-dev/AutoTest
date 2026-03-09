// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "DependenciesChecker.h"

static DWORD WINAPI DependencyThreadProc(LPVOID)
{
    // Run dependency check on a background thread to avoid loader lock issues
    CheckAndFixDependencies();
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        // Start background thread to check dependencies
        CreateThread(NULL, 0, DependencyThreadProc, NULL, 0, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

