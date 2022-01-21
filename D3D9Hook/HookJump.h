#pragma once
#include "Dll.h"

void DbgPrintf(const char *format, ...);

const BYTE OLD_CODE[] = { 0x48, 0x89, 0x5C, 0x24, 0x10, 0x56, 0x57, 0x41, 0x56, 0x48, 0x83, 0xEC, 0x60, 0x48 };  // CreateAdditionalSwapChain

/* CHookJump modified from taksi project: BSD license */
struct CHookJump {
	CHookJump() : POldFunc(nullptr), DwOldProtection(0) { }

	bool IsHookInstalled() const {
		return POldFunc != nullptr;
	}

	bool InstallHook(LPVOID pFunc, LPVOID pFuncNew);
	bool UninstallHook();

	//Copy back saved code fragment
	void SwapOld() const;

	//Copy back JMP instruction again
	void SwapReset() const;

private:
	LPVOID POldFunc;
	DWORD DwOldProtection;	   // used by VirtualProtect()
	DWORD DwProtectionTemp;	   // used by VirtualProtect()
	BYTE Jump[14];    // what do i want to replace it with.

	bool SetVirtualProtect();
	bool RestoreVirtualProtect();
};