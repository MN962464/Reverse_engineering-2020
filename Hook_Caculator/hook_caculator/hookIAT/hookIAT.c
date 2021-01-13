#include "stdio.h"  
#include "wchar.h"  
#include "windows.h"  

typedef BOOL(WINAPI *PFSETWINDOWTEXTW)(HWND hWnd, LPWSTR lpString); //SetWindowsTextW()的地址

// 原函数地址
FARPROC g_pOrginalFunction = NULL;

BOOL WINAPI MySetWindowTextW(HWND hWnd, LPWSTR lpString)
{
	//wchar_t* pNum = L"零一二三四五六七八九";
	wchar_t* pNum = L"零壹贰叁肆伍陆柒捌玖";
	wchar_t temp[2] = { 0, };
	int i = 0, nLen = 0, nIndex = 0;

	nLen = wcslen(lpString);
	for (i = 0; i < nLen; i++)
	{
		//   将阿拉伯数字转换为中文数字  
		//   lpString是宽字符版本(2个字节)字符串  
		if (L'0' <= lpString[i] && lpString[i] <= L'9')
		{
			temp[0] = lpString[i];
			nIndex = _wtoi(temp);
			lpString[i] = pNum[nIndex];
		}
	}

	//   调用原函数；user32.SetWindowTextW  
	//   (修改lpString缓冲区中的内容)  
	return ((PFSETWINDOWTEXTW)g_pOrginalFunction)(hWnd, lpString);
}


BOOL hook_iat(LPCSTR szDllName, PROC pfnOrg, PROC pfnNew)
{
	HMODULE hMod;
	LPCSTR szLibName;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc;
	PIMAGE_THUNK_DATA pThunk;
	DWORD dwOldProtect, dwRVA;
	PBYTE pAddr;

	hMod = GetModuleHandle(NULL);
	pAddr = (PBYTE)hMod;
  
	pAddr += *((DWORD*)&pAddr[0x3C]);

	dwRVA = *((DWORD*)&pAddr[0x80]);

	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)hMod + dwRVA);

	for (; pImportDesc->Name; pImportDesc++)
	{
		// szLibName = VA to IMAGE_IMPORT_DESCRIPTOR.Name  
		szLibName = (LPCSTR)((DWORD)hMod + pImportDesc->Name);
		if (!_stricmp(szLibName, szDllName))
		{
			pThunk = (PIMAGE_THUNK_DATA)((DWORD)hMod +
				pImportDesc->FirstThunk);

			for (; pThunk->u1.Function; pThunk++)
			{
				if (pThunk->u1.Function == (DWORD)pfnOrg)
				{
					// 更改为可读写模式  
					VirtualProtect((LPVOID)&pThunk->u1.Function,
						4,
						PAGE_EXECUTE_READWRITE,
						&dwOldProtect);

					// 修改IAT的值  
					pThunk->u1.Function = (DWORD)pfnNew;

					//修改完成后，恢复原保护属性
					VirtualProtect((LPVOID)&pThunk->u1.Function,
						4,
						dwOldProtect,
						&dwOldProtect);

					return TRUE;
				}
			}
		}
	}

	return FALSE;
}



BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// 保存原始API的地址  
		g_pOrginalFunction = GetProcAddress(GetModuleHandle(L"user32.dll"),
			"SetWindowTextW");

		// # hook  
		//   用hookiat.MySetWindowText钩取user32.SetWindowTextW  
		hook_iat("user32.dll", g_pOrginalFunction, (PROC)MySetWindowTextW);
		break;

	case DLL_PROCESS_DETACH:
		// # unhook  
		//   将calc.exe的IAT恢复原值  
		hook_iat("user32.dll", (PROC)MySetWindowTextW, g_pOrginalFunction);
		break;
	}

	return TRUE;
}
