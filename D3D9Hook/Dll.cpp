#include "HookJump.h"
#include "Dll.h"
#include <d3d9helper.h>
#include <d3dx9tex.h>
#include <cstdio>
#include <mutex>

#define TRY(func) do { auto hr = func; if (FAILED(hr)) return hr; } while(0)

static CHookJump gSwapChainHook;
static HWND targetHwnd;
static UINT targetWidth, targetHeight;
static std::mutex mutex;

static CreateAdditionalSwapChain gCreateAdditionalSwapChain;
static IDirect3DDevice9 *gD3dDevice9 = nullptr;
static IDirect3DSwapChain9 *gD3dSwapChain = nullptr;
static IDirect3DSurface9 *gBackBuffer = nullptr;
static IDirect3DSurface9 *gOffscreenSurface = nullptr;

static bool isCapturePrepared;
static D3DSURFACE_DESC desc;

static void *pixelBuf;

extern "C" __declspec(dllexport) bool WINAPI IsCaptureReady() {
	DbgPrintf("[D3D9Hook] IsCaptureReady: gD3dDevice9: 0x%X, gD3dSwapChain: 0x%X", gD3dDevice9, gD3dSwapChain);
	return gD3dDevice9 != nullptr && gD3dSwapChain != nullptr && isCapturePrepared;
}

/**
 * \brief 将device内容拷贝到buf内
 * \param buf 指向缓冲区的指针
 * \return 成功返回0，错误返回代码
 */
extern "C" __declspec(dllexport) int WINAPI CaptureLock(void **buf) {
	DbgPrintf("[D3D9Hook] Capture start");
	if (!IsCaptureReady()) {
		DbgPrintf("[D3D9Hook] Capture not ready");
		return -1;
	}
	DbgPrintf("[D3D9Hook] GetRenderTargetData");
	if (FAILED(gD3dDevice9->GetRenderTargetData(gBackBuffer, gOffscreenSurface))) {
		DbgPrintf("[D3D9Hook] GetRenderTargetData failed");
		return -2;
	}
	// D3DXSaveSurfaceToFile(".\\1.bmp", D3DXIFF_BMP, gOffscreenSurface, nullptr, nullptr);
	D3DLOCKED_RECT lockedSrcRect;
	DbgPrintf("[D3D9Hook] LockRect");
	if (FAILED(gOffscreenSurface->LockRect(&lockedSrcRect, nullptr, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK | D3DLOCK_NO_DIRTY_UPDATE))) {
		DbgPrintf("[D3D9Hook] LockRect failed");
		return -3;
	}
	*buf = lockedSrcRect.pBits;
	DbgPrintf("[D3D9Hook] CaptureLocked");
	return 0;
}

extern "C" __declspec(dllexport) bool WINAPI CaptureUnlock() {
	if (gOffscreenSurface == nullptr) {
		return false;
	}
	return SUCCEEDED(gOffscreenSurface->UnlockRect());
}

/**
 * \brief 释放所有的资源（如果有）
 */
void Release() {
	if (gOffscreenSurface != nullptr) {
		gOffscreenSurface->Release();
	}
	if (gD3dDevice9 != nullptr) {
		gD3dDevice9->Release();
	}
	if (pixelBuf != nullptr) {
		free(pixelBuf);
	}
}

extern "C" __declspec(dllexport) bool WINAPI Unhook() {
	Release();  // 以防万一，先尝试释放资源
	if (!gSwapChainHook.IsHookInstalled()) {
		return false;
	}
	DbgPrintf("[D3D9Hook] Starting uninstall hook: 0x%X", gCreateAdditionalSwapChain);
	return gSwapChainHook.UninstallHook();
}

/**
 * \brief 为swapChain创建RenderTarget，WPF的device似乎不与窗口直接相关，需要根据swapChain来找
 */
void PrepareForCapture() {
	Release();  // 先尝试释放之前的资源

	// 1. 根据swapChain获取desc，验证大小是否匹配
	if (SUCCEEDED(gD3dSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &gBackBuffer))) {
		gBackBuffer->GetDesc(&desc);
		DbgPrintf("[D3D9Hook] Got desc, width: %d, height: %d, multiSampleType: %d, usage: %d, pool: %d, type: %d, format: %d, mt: %d, mq: %d", desc.Width, desc.Height, desc.MultiSampleType, desc.Usage, desc.Pool, desc.Type, desc.Format, desc.MultiSampleType, desc.MultiSampleQuality);
		if (desc.Width != targetWidth || desc.Height != targetHeight) {
			DbgPrintf("[D3D9Hook] desc size not match");
		} else if (SUCCEEDED(gD3dDevice9->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &gOffscreenSurface, nullptr))) {
			// 2. 根据desc创建一个处于内存中的OffscreenPlainSurface，用于将swapChain中的内容从内存中转移出来存下
			DbgPrintf("[D3D9Hook] CreateOffscreenPlainSurface created: 0x%X", gOffscreenSurface);
			isCapturePrepared = true;
		}
		gBackBuffer->Release();
	}
}

HRESULT WINAPI HookedCreateAdditionalSwapChain(IDirect3DDevice9 *This, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DSwapChain9 **pSwapChain) {
	// DbgPrintf("[D3D9Hook] Hooked CreateAdditionalSwapChain");
	mutex.lock();
	gSwapChainHook.SwapOld();
	const auto hr = gCreateAdditionalSwapChain(This, pPresentationParameters, pSwapChain);
	gSwapChainHook.SwapReset();
	do {
		if (pPresentationParameters->hDeviceWindow != targetHwnd) {
			DbgPrintf("[D3D9Hook] Hooked CreateAdditionalSwapChain, but it's hFocusWindow is: 0x%X", pPresentationParameters->hDeviceWindow);
			break;
		}

		if (pPresentationParameters->BackBufferWidth != targetWidth || pPresentationParameters->BackBufferHeight != targetHeight) {
			DbgPrintf("[D3D9Hook] Hooked CreateAdditionalSwapChain, but it's size is: %dx%d", pPresentationParameters->BackBufferWidth, pPresentationParameters->BackBufferHeight);
			break;
		}

		if (FAILED(hr)) {
			DbgPrintf("[D3D9Hook] Hooked CreateAdditionalSwapChain with target HWND, but the call failed");
			break;
		}
		
		if (*pSwapChain != gD3dSwapChain) {  // 新的SwapChain和旧的不一样
			DbgPrintf("[D3D9Hook] Hooked CreateAdditionalSwapChain succeeded, d3dDevice: 0x%X, swapChain: 0x%X", This, *pSwapChain);
			gD3dDevice9 = This;
			gD3dSwapChain = *pSwapChain;
			isCapturePrepared = false;
			PrepareForCapture();
		}
	} while (false);
	mutex.unlock();
	return hr;
}

/**
 * \brief Hook，之后可以调用
 * \param hwnd DX的窗口
 * \param height
 * \param width
 * \return 成功返回0
 */
extern "C" __declspec(dllexport) HRESULT WINAPI Hook(const HWND hwnd, const UINT width, const UINT height) {
	DbgPrintf("[D3D9Hook] Starting hook, hwnd: 0x%X", hwnd);
	targetHwnd = hwnd;
	targetWidth = width;
	targetHeight = height;
	if (gSwapChainHook.IsHookInstalled()) {
		DbgPrintf("[D3D9Hook] Hook installed, return");
		return 1;
	}

	const auto hD3D9 = reinterpret_cast<ULONG_PTR>(GetModuleHandle("d3d9.dll"));  // 模块句柄就是模块地址
	gCreateAdditionalSwapChain = reinterpret_cast<CreateAdditionalSwapChain>(hD3D9 + 0x42FB0);

	gD3dDevice9 = nullptr;
	gD3dSwapChain = nullptr;
	isCapturePrepared = false;

	DbgPrintf("[D3D9Hook] Starting install hook: 0x%X --> 0x%X", gCreateAdditionalSwapChain, HookedCreateAdditionalSwapChain);
	if (!gSwapChainHook.InstallHook(gCreateAdditionalSwapChain, HookedCreateAdditionalSwapChain)) {
		return 2;
	}

	DbgPrintf("[D3D9Hook] Hook installed");
	return S_OK;
}

BOOL APIENTRY DllMain(HMODULE hModule, const DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH) {
		DbgPrintf("[D3D9Hook] DLL_PROCESS_ATTACH");
	}
	return TRUE;
}