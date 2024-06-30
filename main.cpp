#include <Windows.h>
#include <stdio.h>
#include "..\BoosterDrv\BoosterCommon.h"

int main(int argc, const char* argv[])
{
	if (argc < 3)
	{
		printf("Usage: %s <tid> <priority>\n", argv[0]);
		return 0;
	}

	int tid = atoi(argv[1]);
	int priority = atoi(argv[2]);

	HANDLE hDevice = CreateFile(L"\\\\.\\Booster", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Error opening device handle: %u", GetLastError());
		return 1;
	}

	ThreadData data;
	data.ThreadId = tid;
	data.Priority = priority;

	DWORD ret;

	BOOL ok = WriteFile(hDevice, &data, sizeof(data), &ret, nullptr);

	if (!ok)
	{
		printf("Error writing to device: %u", GetLastError());
		CloseHandle(hDevice);
		return 1;
	}

	printf("Success!!\n");
	CloseHandle(hDevice);


	return 0;
}