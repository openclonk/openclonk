/*
nsisFirewall -- Small NSIS plugin for simple tasks with Windows Firewall
Web site: http://wiz0u.free.fr/prog/nsisFirewall

Copyright (c) 2007-2009 Olivier Marcoux

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/
#undef UNICODE
#undef _UNICODE
#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>

#include <initguid.h>
#include "netfw.h"
#include "exdll.h"

HRESULT AddAuthorizedApplication(wchar_t * ExceptionName, wchar_t * ProcessPath)
{
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
		return hr;

	INetFwMgr * mgr = 0;
	INetFwAuthorizedApplication * app = 0;
	INetFwPolicy * localPolicy = 0;
	INetFwProfile * profile = 0;
	INetFwAuthorizedApplications * apps = 0;

	BSTR bstrRuleName = SysAllocString(ExceptionName);
	BSTR bstrApplicationName = SysAllocString(ProcessPath);

	hr = CoCreateInstance(CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER, IID_INetFwMgr,
		reinterpret_cast<void**>(static_cast<INetFwMgr**>(&mgr)));
	if (FAILED(hr)) goto out;

	hr = CoCreateInstance(CLSID_NetFwAuthorizedApplication, NULL, CLSCTX_INPROC_SERVER, IID_INetFwAuthorizedApplication,
		reinterpret_cast<void**>(static_cast<INetFwAuthorizedApplication**>(&app)));
	if (FAILED(hr)) goto out;

	app->put_ProcessImageFileName(bstrApplicationName);
	app->put_Name(bstrRuleName);
	app->put_Scope(NET_FW_SCOPE_ALL);
	app->put_IpVersion(NET_FW_IP_VERSION_ANY);
	app->put_Enabled(VARIANT_TRUE);

	hr = mgr->get_LocalPolicy(&localPolicy);
	if (FAILED(hr)) goto out;

	hr = localPolicy->get_CurrentProfile(&profile);
	if (FAILED(hr)) goto out;

	hr = profile->get_AuthorizedApplications(&apps);
	if (FAILED(hr)) goto out;

	hr = apps->Add(app);

out:
	SysFreeString(bstrRuleName);
	SysFreeString(bstrApplicationName);
	if(apps) apps->Release();
	if(profile) profile->Release();
	if(localPolicy) localPolicy->Release();
	if(app) app->Release();
	if(mgr) mgr->Release();
	CoUninitialize();
	return hr;
}

HRESULT RemoveAuthorizedApplication(wchar_t * ProcessPath)
{
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) return hr;

	INetFwMgr * mgr = 0;
	INetFwPolicy * localPolicy = 0;
	INetFwProfile * profile = 0;
	INetFwAuthorizedApplications * apps = 0;

	BSTR bstrApplicationName = SysAllocString(ProcessPath);

	hr = CoCreateInstance(CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER, IID_INetFwMgr,
		reinterpret_cast<void**>(static_cast<INetFwMgr**>(&mgr)));
	if (FAILED(hr)) goto out;

	hr = mgr->get_LocalPolicy(&localPolicy);
	if (FAILED(hr)) goto out;

	hr = localPolicy->get_CurrentProfile(&profile);
	if (FAILED(hr)) goto out;

	hr = profile->get_AuthorizedApplications(&apps);
	if (FAILED(hr)) goto out;

	hr = apps->Remove(bstrApplicationName);

out:
	SysFreeString(bstrApplicationName);
	if(apps) apps->Release();
	if(profile) profile->Release();
	if(localPolicy) localPolicy->Release();
	if(mgr) mgr->Release();
	CoUninitialize();
	return hr;
}

extern "C" void __declspec(dllexport) AddAuthorizedApplication(HWND hwndParent, int string_size, 
                                      TCHAR *variables, stack_t **stacktop)
{
	EXDLL_INIT();

	wchar_t ExceptionName[256], ProcessPath[MAX_PATH];
	popstring(ProcessPath, MAX_PATH);
	popstring(ExceptionName, 256);
	HRESULT result = AddAuthorizedApplication(ExceptionName, ProcessPath);
	// push the result back to NSIS
	TCHAR intBuffer[16];
	wsprintf(intBuffer, _T("%d"), result);
	pushstring(intBuffer);
}

extern "C" void __declspec(dllexport) RemoveAuthorizedApplication(HWND hwndParent, int string_size, 
                                      TCHAR *variables, stack_t **stacktop)
{
	EXDLL_INIT();

	wchar_t ProcessPath[MAX_PATH];
	popstring(ProcessPath, MAX_PATH);
	HRESULT result = RemoveAuthorizedApplication(ProcessPath);
	// push the result back to NSIS
	TCHAR intBuffer[16];
	wsprintf(intBuffer, _T("%d"), result);
	pushstring(intBuffer);
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD, LPVOID)
{
	return TRUE;
}

