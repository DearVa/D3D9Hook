#include "HookJump.h"

#include <cstdio>

bool CHookJump::SetVirtualProtect() {
	if (!VirtualProtect(POldFunc, sizeof(POldFunc), PAGE_READWRITE, &DwOldProtection)) {
		DbgPrintf("[D3D9Hook] VirtualProtect fault");
		return false;
	}
	return true;
}

bool CHookJump::RestoreVirtualProtect() {
	if (!VirtualProtect(POldFunc, sizeof(POldFunc), DwOldProtection, &DwProtectionTemp)) {
		DbgPrintf("[D3D9Hook] VirtualProtect restore fault, DwOldProtection: %d", DwOldProtection);
		return false;
	}
	return true;
}

/* CHookJump modified from taksi project: BSD license */
bool CHookJump::InstallHook(LPVOID pFunc, LPVOID pFuncNew) {
	if (pFunc == nullptr) {
		return false;
	}

	if (IsHookInstalled() && !memcmp(pFunc, Jump, sizeof(Jump))) {
		return true;
	}
	
	/*if (memcmp(pFunc, OLD_CODE, sizeof(OLD_CODE)) != 0) {
		const auto func = static_cast<BYTE*>(pFunc);
		DbgPrintf("[D3D9Hook] Old Code not match, expected: 48 89 57 24 8, but got: %X %X %X %X %X", func[0], func[1], func[2], func[3], func[4]);
		return false;
	}*/

	// unconditional JMP to relative address is 5 bytes
	Jump[0] = 0xE9;
	const DWORD dwAddr = static_cast<DWORD>(reinterpret_cast<UINT_PTR>(pFuncNew) - reinterpret_cast<UINT_PTR>(pFunc)) - sizeof(Jump);
	memcpy(Jump + 1, &dwAddr, sizeof(dwAddr));

	POldFunc = pFunc;
	if (!SetVirtualProtect()) {
		return false;
	}
	memcpy(OldCode, pFunc, sizeof(OldCode));
	DbgPrintf("[D3D9Hook] OldCode: %X %X %X %X %X", OldCode[0], OldCode[1], OldCode[2], OldCode[3], OldCode[4]);
	memcpy(pFunc, Jump, sizeof(Jump));
	RestoreVirtualProtect();  // 不及时改回去有时会导致.NET报错

	return true;
}

bool CHookJump::UninstallHook() {
	if (POldFunc == nullptr) {
		return false;
	}

	/* was never set! */
	if (!IsHookInstalled()) {
		return false;
	}

	if (!SetVirtualProtect()) {
		return false;
	}
	memcpy(POldFunc, OldCode, sizeof(OldCode));  // SwapOld(pFunc)
	if (!RestoreVirtualProtect()) {
		return false;
	}
	return true;
}

void CHookJump::SwapOld() {
	if (SetVirtualProtect()) {
		memcpy(POldFunc, OldCode, sizeof(OldCode));
		RestoreVirtualProtect();
	}
}

void CHookJump::SwapReset() {
	if (!IsHookInstalled()) {
		return;
	}
	if (SetVirtualProtect()) {
		memcpy(POldFunc, Jump, sizeof(Jump));
		RestoreVirtualProtect();
	}
}


void DbgPrintf(const char *format, ...) {
	static char buf[1024];
	va_list args;
	va_start(args, format);
	_vsnprintf_s(buf, sizeof(buf), format, args);
	va_end(args);
	OutputDebugStringA(buf);
	printf("%s\n\r", buf);
}
