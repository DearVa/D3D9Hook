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

bool CHookJump::InstallHook(LPVOID pFunc, LPVOID pFuncNew) {
	if (pFunc == nullptr || pFuncNew == nullptr) {
		return false;
	}

	if (IsHookInstalled() && memcmp(pFunc, Jump, sizeof(Jump)) == 0) {
		return true;
	}

	// unconditional JMP to relative address is 5 bytes
	Jump[0] = 0xE9;
	const DWORD dwAddr = static_cast<DWORD>(reinterpret_cast<UINT_PTR>(pFuncNew) - reinterpret_cast<UINT_PTR>(pFunc)) - sizeof(Jump);
	memcpy(Jump + 1, &dwAddr, sizeof(dwAddr));

	POldFunc = pFunc;
	if (!SetVirtualProtect()) {
		POldFunc = nullptr;
		return false;
	}

	if (memcmp(pFunc, OLD_CODE, sizeof(OLD_CODE)) != 0) {
		RestoreVirtualProtect();
		POldFunc = nullptr;
		const auto func = static_cast<BYTE *>(pFunc);
		DbgPrintf("[D3D9Hook] Old Code not match, expected: 48 89 5C 24 10, but got: %X %X %X %X %X", func[0], func[1], func[2], func[3], func[4]);
		return false;
	}

	memcpy(pFunc, Jump, sizeof(Jump));
	RestoreVirtualProtect();  // 不及时改回去有时会导致.NET报错
	return true;
}

bool CHookJump::UninstallHook() {
	/* was never set! */
	if (!IsHookInstalled()) {
		return false;
	}

	if (!SetVirtualProtect()) {
		return false;
	}
	memcpy(POldFunc, OLD_CODE, sizeof(OLD_CODE));  // SwapOld(pFunc)
	if (!RestoreVirtualProtect()) {
		return false;
	}
	return true;
}

void CHookJump::SwapOld() {
	if (IsHookInstalled() && SetVirtualProtect()) {
		memcpy(POldFunc, OLD_CODE, sizeof(OLD_CODE));
		RestoreVirtualProtect();
	}
}

void CHookJump::SwapReset() {
	if (IsHookInstalled() && SetVirtualProtect()) {
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
