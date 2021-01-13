#include<Windows.h>
#include<iostream>
#include<stdio.h>

using namespace std;

LPVOID WriteFileAddress = NULL;   //
CREATE_PROCESS_DEBUG_INFO CreateProcessDebugInfomation;
BYTE INT3 = 0xCC, OldByte = 0;

BOOL OnCreateProcessDebugEvent(LPDEBUG_EVENT pDebugEvent)
{
	// WriteFile()������ַ
	WriteFileAddress = GetProcAddress(GetModuleHandleA("kernel32.dll"), "WriteFile");//���WriteFile�ĵ�ַ

	//��WriteFile�������׸��ֽڸ�Ϊ0xcc
	memcpy(&CreateProcessDebugInfomation, &pDebugEvent->u.CreateProcessInfo, sizeof(CREATE_PROCESS_DEBUG_INFO));

	//����ԭ�����׵�ַ�����ֽ�
	ReadProcessMemory(CreateProcessDebugInfomation.hProcess, WriteFileAddress, &OldByte, sizeof(BYTE), NULL);

	//д��0xCC���¶ϵ�
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
		// �жϷ����쳣�ĵط��Ƿ�ΪҪ��ȡ�ĺ���
		if (WriteFileAddress == pExceptionRecord->ExceptionAddress)
		{
			//  Unhook
			WriteProcessMemory(CreateProcessDebugInfomation.hProcess, WriteFileAddress,
				&OldByte, sizeof(BYTE), NULL);

			// ����߳����±�����
			Context.ContextFlags = CONTEXT_CONTROL;
			GetThreadContext(CreateProcessDebugInfomation.hThread, &Context);

			// WriteFile() ����ESP�����WriteFile �����Ĳ������Դﵽ�޸����ݵ�Ŀ��

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

			// �޸�����
			for (i = 0; i < dwNumOfBytesToWrite; i++)
			{
				if (0x61 <= lpBuffer[i] && lpBuffer[i] <= 0x7A)
					lpBuffer[i] -= 0x20;
			}

			printf("\n### converted string ###\n%s\n", lpBuffer);

			// #7. ����ԭ����
			WriteProcessMemory(CreateProcessDebugInfomation.hProcess, (LPVOID)dwAddrOfBuffer,
				lpBuffer, dwNumOfBytesToWrite, NULL);


			free(lpBuffer);

			// ����EIP��ֵ��ʵ���������У�ע��EIP��ֵΪ0xcc����һ��ָ��ĵ�ַ��
			Context.Eip = (DWORD)WriteFileAddress;
			SetThreadContext(CreateProcessDebugInfomation.hThread, &Context);

			// ����
			ContinueDebugEvent(pDebugEvent->dwProcessId, pDebugEvent->dwThreadId, DBG_CONTINUE);
			Sleep(0);

			// �ٴι�ȡ
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

	// �ȴ������¼�
	while (WaitForDebugEvent(&DebugEvent, INFINITE))
	{
		dwContinueStatus = DBG_CONTINUE;

		// �����¼�Ϊ��������
		if (CREATE_PROCESS_DEBUG_EVENT == DebugEvent.dwDebugEventCode)
		{
			OnCreateProcessDebugEvent(&DebugEvent);
		}
		// �����¼�
		else if (EXCEPTION_DEBUG_EVENT == DebugEvent.dwDebugEventCode)
		{
			if (OnExceptionDebugEvent(&DebugEvent))
				continue;
		}
		// ���Խ����˳�
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

	// �����¼�ѭ��
	DebugLoop();

	return 0;
}
