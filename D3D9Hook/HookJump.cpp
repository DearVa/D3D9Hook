#include "HookJump.h"

#include <cstdio>

/* CHookJump modified from taksi project: BSD license */
bool CHookJump::InstallHook(LPVOID pFunc, LPVOID pFuncNew) {
	if (pFunc == nullptr) {
		return false;
	}

	if (IsHookInstalled() && !memcmp(pFunc, Jump, sizeof(Jump))) {
		return true;
	}

	Jump[0] = 0;
	
	if (!VirtualProtect(pFunc, sizeof(pFunc), PAGE_EXECUTE_READWRITE, &DwOldProtection)) {
		DbgPrintf("[D3D9Hook] VirtualProtect fault");
		return false;
	}

	if (memcmp(pFunc, OLD_CODE, sizeof(OLD_CODE)) != 0) {
		const auto func = static_cast<BYTE*>(pFunc);
		DbgPrintf("[D3D9Hook] Old Code not match, expected: 48 83 C1 C8 E9, but got: %X %X %X %X %X", func[0], func[1], func[2], func[3], func[4]);
		return false;
	}

	/* unconditional JMP to relative address is 5 bytes */
	Jump[0] = 0xE9;
	const DWORD dwAddr = static_cast<DWORD>(reinterpret_cast<UINT_PTR>(pFuncNew) - reinterpret_cast<UINT_PTR>(pFunc)) - sizeof(Jump);
	memcpy(Jump + 1, &dwAddr, sizeof(dwAddr));
	memcpy(pFunc, Jump, sizeof(Jump));

	return true;
}

bool CHookJump::UninstallHook(const LPVOID pFunc) {
	if (pFunc == nullptr) {
		return false;
	}

	/* was never set! */
	if (!IsHookInstalled()) {
		return false;
	}

	memcpy(pFunc, OLD_CODE, sizeof(OLD_CODE));  // SwapOld(pFunc)
	if (!VirtualProtect(pFunc, sizeof(pFunc), DwOldProtection, &DwOldProtection)) {  // restore protection.
		return false;
	}
	Jump[0] = 0;
	return true;
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