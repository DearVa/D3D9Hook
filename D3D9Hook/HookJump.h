#pragma once
#include "Dll.h"

void DbgPrintf(const char *format, ...);

const BYTE OLD_CODE[] = { 0x48, 0x83, 0xC1, 0xC8, 0xE9 };

/* CHookJump modified from taksi project: BSD license */
struct CHookJump {
	CHookJump() : DwOldProtection(0) {
		Jump[0] = 0;
	}

	bool IsHookInstalled() const {
		return Jump[0] != 0;
	}

	bool InstallHook(LPVOID pFunc, LPVOID pFuncNew);
	bool UninstallHook(LPVOID pFunc);

	//Copy back saved code fragment
	void SwapOld(const LPVOID pFunc) const {
		memcpy(pFunc, OLD_CODE, sizeof(OLD_CODE));
	}

	//Copy back JMP instruction again
	void SwapReset(const LPVOID pFunc) const {
		/* hook has since been destroyed! */
		if (!IsHookInstalled())
			return;

		memcpy(pFunc, Jump, sizeof(Jump));
	}

private:
	DWORD DwOldProtection;	   // used by VirtualProtect()
	BYTE Jump[5];    // what do i want to replace it with.
};