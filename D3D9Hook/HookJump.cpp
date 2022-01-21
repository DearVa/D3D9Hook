#include "HookJump.h"

#include <cstdio>

bool CHookJump::SetVirtualProtect() {
	if (!VirtualProtect(POldFunc, 14, PAGE_EXECUTE_READWRITE, &DwOldProtection)) {
		DbgPrintf("[D3D9Hook] VirtualProtect fault");
		return false;
	}
	return true;
}

bool CHookJump::RestoreVirtualProtect() {
	if (!VirtualProtect(POldFunc, 14, DwOldProtection, &DwProtectionTemp)) {
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

	// unconditional JMP to far address is 14 bytes
	auto dwTo = reinterpret_cast<DWORD_PTR>(pFuncNew);
	Jump[0] = 0x68;
	const auto dJump = reinterpret_cast<DWORD32*>(Jump + 1);
	dJump[0] = dwTo & 0xffffffff;
	dJump[1] = 0x042444c7;
	dJump[2] = dwTo >> 32;
	Jump[13] = 0xc3;

	POldFunc = pFunc;
	if (!SetVirtualProtect()) {
		POldFunc = nullptr;
		return false;
	}

	if (memcmp(pFunc, OLD_CODE, sizeof(OLD_CODE)) != 0) {
		RestoreVirtualProtect();
		POldFunc = nullptr;
		const auto func = static_cast<BYTE *>(pFunc);
		DbgPrintf("[D3D9Hook] Old Code not match, expected: 0x48, 0x89, 0x5C, 0x24, 0x10, 0x56, 0x57, 0x41, 0x56, 0x48, 0x83, 0xEC, 0x60, 0x48,"
			"but got: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X", func[0], func[1], func[2], func[3], func[4], func[5], func[6], 
			func[7], func[8], func[9], func[10], func[11], func[12], func[13]);
		return false;
	}

	memcpy(pFunc, Jump, sizeof(Jump));
	return true;
}

bool CHookJump::UninstallHook() {
	/* was never set! */
	if (!IsHookInstalled()) {
		return false;
	}

	memcpy(POldFunc, OLD_CODE, sizeof(OLD_CODE));  // SwapOld(pFunc)
	RestoreVirtualProtect();
	return true;
}

void CHookJump::SwapOld() const {
	if (IsHookInstalled()) {
		memcpy(POldFunc, OLD_CODE, sizeof(OLD_CODE));
	}
}

void CHookJump::SwapReset() const {
	if (IsHookInstalled()) {
		memcpy(POldFunc, Jump, sizeof(Jump));
	}
}


void DbgPrintf(const char *format, ...) {
	/*static char buf[1024];
	va_list args;
	va_start(args, format);
	_vsnprintf_s(buf, sizeof(buf), format, args);
	va_end(args);
	OutputDebugStringA(buf);
	printf("%s\n\r", buf);*/
}
