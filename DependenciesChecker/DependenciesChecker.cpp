#include "pch.h"
#include "DependenciesChecker.h"
#include <shellapi.h>
#include <string>

static bool IsProcessElevated()
{
    BOOL bElevated = FALSE;
    HANDLE token = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        TOKEN_ELEVATION elevation;
        DWORD retLen = 0;

        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &retLen))
        {
            bElevated = elevation.TokenIsElevated;
        }

        CloseHandle(token);
    }

    return bElevated == TRUE;
}

static bool FindExecutableOnPath(LPCWSTR exeName, std::wstring &foundPath)
{
    wchar_t buf[MAX_PATH] = { 0 };
    DWORD len = SearchPathW(NULL, exeName, NULL, MAX_PATH, buf, NULL);

    if (len > 0 && len < MAX_PATH)
    {
        foundPath = buf;
        return true;
    }

    return false;
}

static HRESULT TryRelaunchElevated()
{
    wchar_t exePath[MAX_PATH] = { 0 };

    if (GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    HINSTANCE result = ShellExecuteW(NULL, L"runas", exePath, NULL, NULL, SW_SHOWNORMAL);

    if ((INT_PTR)result > 32)
    {
        return S_OK;
    }

    return HRESULT_FROM_WIN32(ERROR_CANCELLED);
}

extern "C" __declspec(dllexport) HRESULT CheckAndFixDependencies()
{
    bool hasWpr = false;
    bool hasWpa = false;
    std::wstring path;

    hasWpr = FindExecutableOnPath(L"wpr.exe", path);
    hasWpa = FindExecutableOnPath(L"wpa.exe", path);

    bool elevated = IsProcessElevated();

    if (hasWpr && hasWpa && elevated)
    {
        return S_OK;
    }

    if (!elevated)
    {
        HRESULT hr = TryRelaunchElevated();

        if (SUCCEEDED(hr))
        {
            return S_FALSE;
        }
    }

    if (!hasWpr || !hasWpa)
    {
        std::wstring msg = L"Required dependencies not found:\n";

        if (!hasWpr)
        {
            msg += L" - wpr.exe (Windows Performance Recorder)\n";
        }

        if (!hasWpa)
        {
            msg += L" - wpa.exe (Windows Performance Analyzer)\n\n";
        }

        msg += L"Please install the Windows Performance Toolkit (part of the Windows ADK) and try again.";
        MessageBoxW(NULL, msg.c_str(), L"Dependencies Missing", MB_ICONERROR | MB_OK);
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    MessageBoxW(NULL, L"Administrator privileges are required. Please restart the application as an administrator.", L"Elevation Required", MB_ICONERROR | MB_OK);
    return HRESULT_FROM_WIN32(ERROR_ELEVATION_REQUIRED);
}
