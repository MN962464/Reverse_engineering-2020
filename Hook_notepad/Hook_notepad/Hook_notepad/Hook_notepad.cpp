#include<Windows.h>
#include<iostream>
#include<stdio.h>

using namespace std;

LPVOID WriteFileAddress = NULL;   //
CREATE_PROCESS_DEBUG_INFO CreateProcessDebugInfomation;
BYTE INT3 = 0xCC, OldByte = 0;

BOOL OnCreateProcessDebugEvent(LPDEBUG_EVENT pDebugEvent)
{
	// WriteFile()函数地址
	WriteFileAddress = GetProcAddress(GetModuleHandleA("kernel32.dll"), "WriteFile");//获得WriteFile的地址

	//将WriteFile函数的首个字节改为0xcc
	memcpy(&CreateProcessDebugInfomation, &pDebugEvent->u.CreateProcessInfo, sizeof(CREATE_PROCESS_DEBUG_INFO));

	//保存原函数首地址的首字节
	ReadProcessMemory(CreateProcessDebugInfomation.hProcess, WriteFileAddress, &OldByte, sizeof(BYTE), NULL);

	//写入0xCC，下断点
	WriteProcessMemory(CreateProcessDebugInfomation.hProcess, WriteFileAddress, &INT3, sizeof(BYTE), NULL);

	return TRUE;
}

BOOL OnExceptionDebugEvent(LPDEBUG_EVENT pDebugEvent)
{
	CONTEXT Context;
	PBYTE lpBuffer = NULL;
	DWORD dwNumOfBytesToWrite, dwAddrOfBuffer, i;
	PEXCEPTION_RECORD pExceptionRecord = &pDebugEvent->u.Exception.ExceptionRecord;

	// BreakPoint exception 
	if (EXCEPTION_BREAKPOINT == pExceptionRecord->ExceptionCode)
	{
		// 判断发生异常的地方是否为要钩取的函数
		if (WriteFileAddress == pExceptionRecord->ExceptionAddress)
		{
			//  Unhook
			WriteProcessMemory(CreateProcessDebugInfomation.hProcess, WriteFileAddress,
				&OldByte, sizeof(BYTE), NULL);

			// 获得线程上下背景文
			Context.ContextFlags = CONTEXT_CONTROL;
			GetThreadContext(CreateProcessDebugInfomation.hThread, &Context);

			// WriteFile() 根据ESP来获得WriteFile 函数的参数，以达到修改数据的目的

			ReadProcessMemory(CreateProcessDebugInfomation.hProcess, (LPVOID)(Context.Esp + 0x8),
				&dwAddrOfBuffer, sizeof(DWORD), NULL);
			ReadProcessMemory(CreateProcessDebugInfomation.hProcess, (LPVOID)(Context.Esp + 0xC),
				&dwNumOfBytesToWrite, sizeof(DWORD), NULL);

		
			lpBuffer = (PBYTE)malloc(dwNumOfBytesToWrite + 1);
			memset(lpBuffer, 0, dwNumOfBytesToWrite + 1);

			// WriteFile() 
			ReadProcessMemory(CreateProcessDebugInfomation.hProcess, (LPVOID)dwAddrOfBuffer,
				lpBuffer, dwNumOfBytesToWrite, NULL);
			printf("\n### original string ###\n%s\n", lpBuffer);

			// 修改数据
			for (i = 0; i < dwNumOfBytesToWrite; i++)
			{
				if (0x61 <= lpBuffer[i] && lpBuffer[i] <= 0x7A)
					lpBuffer[i] -= 0x20;
			}

			printf("\n### converted string ###\n%s\n", lpBuffer);

			// #7. 调用原函数
			WriteProcessMemory(CreateProcessDebugInfomation.hProcess, (LPVOID)dwAddrOfBuffer,
				lpBuffer, dwNumOfBytesToWrite, NULL);


			free(lpBuffer);

			// 设置EIP的值来实现正常运行，注意EIP的值为0xcc的下一条指令的地址。
			Context.Eip = (DWORD)WriteFileAddress;
			SetThreadContext(CreateProcessDebugInfomation.hThread, &Context);

			// 运行
			ContinueDebugEvent(pDebugEvent->dwProcessId, pDebugEvent->dwThreadId, DBG_CONTINUE);
			Sleep(0);

			// 再次钩取
			WriteProcessMemory(CreateProcessDebugInfomation.hProcess, WriteFileAddress,
				&INT3, sizeof(BYTE), NULL);

			return TRUE;
		}
	}

	return FALSE;
}

void DebugLoop()
{
	DEBUG_EVENT DebugEvent;
	DWORD dwContinueStatus;

	// 等待调试事件
	while (WaitForDebugEvent(&DebugEvent, INFINITE))
	{
		dwContinueStatus = DBG_CONTINUE;

		// 调试事件为创建进程
		if (CREATE_PROCESS_DEBUG_EVENT == DebugEvent.dwDebugEventCode)
		{
			OnCreateProcessDebugEvent(&DebugEvent);
		}
		// 调试事件
		else if (EXCEPTION_DEBUG_EVENT == DebugEvent.dwDebugEventCode)
		{
			if (OnExceptionDebugEvent(&DebugEvent))
				continue;
		}
		// 调试进程退出
		else if (EXIT_PROCESS_DEBUG_EVENT == DebugEvent.dwDebugEventCode)
		{

			break;
		}

		ContinueDebugEvent(DebugEvent.dwProcessId, DebugEvent.dwThreadId, dwContinueStatus);
	}
}

int main(int argc, char* argv[])
{
	DWORD dwProcessID;
	cout << "Input ProcessID" << endl;
	cin >> dwProcessID;

	// Attach Process

	if (!DebugActiveProcess(dwProcessID))
	{
		printf("DebugActiveProcess(%d) failed!!!\n"
			"Error Code = %d\n", dwProcessID, GetLastError());
		return 1;
	}

	// 调试事件循环
	DebugLoop();

	return 0;
}
