#pragma once
#include "Dll.h"

void DbgPrintf(const char *format, ...);

// const BYTE OLD_CODE[] = { 0x48, 0x83, 0xC1, 0xC8, 0xE9 };  // SwapChain.Present
// const BYTE OLD_CODE[] = { 0x48, 0x89, 0x5C, 0x24, 0x8 };  // CreateDeviceEx

/* CHookJump modified from taksi project: BSD license */
struct CHookJump {
	CHookJump() : DwOldProtection(0) {
		OldCode[0] = 0;
	}

	bool IsHookInstalled() const {
		return OldCode[0] != 0;
	}

	bool InstallHook(LPVOID pFunc, LPVOID pFuncNew);
	bool UninstallHook();

	//Copy back saved code fragment
	void SwapOld();

	//Copy back JMP instruction again
	void SwapReset();

private:
	LPVOID POldFunc;
	DWORD DwOldProtection;	   // used by VirtualProtect()
	DWORD DwProtectionTemp;	   // used by VirtualProtect()
	BYTE OldCode[5];
	BYTE Jump[5];    // what do i want to replace it with.

	bool SetVirtualProtect();
	bool RestoreVirtualProtect();
};