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

static CreateDevice gCreateDevice;
static CreateDeviceEx gCreateDeviceEx;
static CreateAdditionalSwapChain gCreateAdditionalSwapChain;
static IDirect3DDevice9 *gD3dDevice9 = nullptr;
static IDirect3DSwapChain9 *gD3dSwapChain = nullptr;

static bool isCapturePrepared;
static D3DSURFACE_DESC desc;
static LPDIRECT3DSURFACE9 gOffscreenSurface;

static void *pixelBuf;

/**
 * \brief 将device内容拷贝到buf内
 * \param width
 * \param height
 * \param buf
 * \return
 */
extern "C" __declspec(dllexport) int WINAPI Capture(UINT * width, UINT * height, void **buf) {
	DbgPrintf("[D3D9Hook] Capture start");
	if (gD3dDevice9 == nullptr || gD3dSwapChain == nullptr) {
		DbgPrintf("[D3D9Hook] Capture not ready");
		return 1;
	}
	IDirect3DSurface9 *backBuffer;
	if (SUCCEEDED(gD3dSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer))) {
		backBuffer->GetDesc(&desc);
	}
	if (!isCapturePrepared) {
		//PrepareForCapture();
	}
	DbgPrintf("[D3D9Hook] CreateOffscreenPlainSurface");
	if (FAILED(gD3dDevice9->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &gOffscreenSurface, nullptr))) {
		DbgPrintf("[D3D9Hook] CreateOffscreenPlainSurface failed");
		return 2;
	}
	DbgPrintf("[D3D9Hook] GetRenderTargetData");
	if (FAILED(gD3dDevice9->GetRenderTargetData(backBuffer, gOffscreenSurface))) {
		DbgPrintf("[D3D9Hook] GetRenderTargetData failed");
		return 3;
	}
	D3DXSaveSurfaceToFile(".\\1.bmp", D3DXIFF_BMP, gOffscreenSurface, nullptr, nullptr);
	D3DLOCKED_RECT lockedSrcRect;
	DbgPrintf("[D3D9Hook] LockRect");
	if (FAILED(gOffscreenSurface->LockRect(&lockedSrcRect, nullptr, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK | D3DLOCK_NO_DIRTY_UPDATE))) {
		DbgPrintf("[D3D9Hook] LockRect failed");
		return 4;
	}
	*width = desc.Width;
	*height = desc.Height;
	pixelBuf = malloc(static_cast<size_t>(desc.Width) * desc.Height * 4);
	DbgPrintf("[D3D9Hook] Width: %d, Height: %d, Pitch: %d", desc.Width, desc.Height, lockedSrcRect.Pitch);
	memcpy(pixelBuf, lockedSrcRect.pBits, static_cast<size_t>(lockedSrcRect.Pitch) * desc.Height);
	*buf = pixelBuf;
	gOffscreenSurface->UnlockRect();
	gOffscreenSurface->Release();
	auto pBuf = static_cast<BYTE*>(pixelBuf);
	DbgPrintf("[D3D9Hook] Captured: %X %X %X %X %X %X %X %X", pBuf[0], pBuf[1], pBuf[2], pBuf[3], pBuf[4], pBuf[5], pBuf[6], pBuf[7]);
	return 0;
}

extern "C" __declspec(dllexport) bool WINAPI IsCaptureReady() {
	DbgPrintf("[D3D9Hook] IsCaptureReady: gD3dDevice9: 0x%X, gD3dSwapChain: 0x%X", gD3dDevice9, gD3dSwapChain);
	return gD3dDevice9 != nullptr && gD3dSwapChain != nullptr;
}

HRESULT WINAPI HookedCreateDevice(IDirect3D9 *This, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DDevice9 **ppReturnedDeviceInterface) {
	mutex.lock();
	gSwapChainHook.SwapOld();
	const auto hr = gCreateDevice(This, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
	gSwapChainHook.SwapReset();

	do {
		if (hFocusWindow != targetHwnd) {
			DbgPrintf("[D3D9Hook] Hooked CreateDevice, but it's hFocusWindow: 0x%X not match target HWND", hFocusWindow);
			break;
		}

		if (FAILED(hr)) {
			DbgPrintf("[D3D9Hook] Hooked CreateDevice with target HWND, but the call failed");
			break;
		}

		if (*ppReturnedDeviceInterface != gD3dDevice9) {  // 新的device和旧的不一样
			DbgPrintf("[D3D9Hook] Hooked CreateDevice succeeded, Preparing for capture, d3dDevice: 0x%X", *ppReturnedDeviceInterface);
			// PrepareForCapture(*ppReturnedDeviceInterface);
		}
	} while (false);
	// 调用原始函数并返回
	mutex.unlock();
	return hr;
}

HRESULT WINAPI HookedCreateDeviceEx(IDirect3D9Ex *This, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters, D3DDISPLAYMODEEX *pFullscreenDisplayMode, IDirect3DDevice9Ex **ppReturnedDeviceInterface) {
	DbgPrintf("[D3D9Hook] Hooked CreateDevice");
	mutex.lock();
	gSwapChainHook.SwapOld();
	const auto hr = gCreateDeviceEx(This, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, pFullscreenDisplayMode, ppReturnedDeviceInterface);
	gSwapChainHook.SwapReset();

	do {
		if (hFocusWindow != targetHwnd) {
			DbgPrintf("[D3D9Hook] Hooked CreateDevice, but it's hFocusWindow is: 0x%X", hFocusWindow);
			break;
		}

		if (FAILED(hr)) {
			DbgPrintf("[D3D9Hook] Hooked CreateDevice with target HWND, but the call failed");
			break;
		}

		if (*ppReturnedDeviceInterface != gD3dDevice9) {  // 新的device和旧的不一样
			DbgPrintf("[D3D9Hook] Hooked CreateDevice succeeded, Preparing for capture, d3dDevice: 0x%X", *ppReturnedDeviceInterface);
			// PrepareForCapture(*ppReturnedDeviceInterface);
		}
	} while (false);
	// 调用原始函数并返回
	mutex.unlock();
	return hr;
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
	
	IDirect3DSurface9 *backBuffer;
	// 1. 根据swapChain获取desc，验证大小是否匹配
	if (SUCCEEDED(gD3dSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer))) {
		backBuffer->GetDesc(&desc);
		DbgPrintf("[D3D9Hook] Got desc, width: %d, height: %d, multiSampleType: %d, usage: %d, pool: %d, type: %d, format: %d, mt: %d, mq: %d", desc.Width, desc.Height, desc.MultiSampleType, desc.Usage, desc.Pool, desc.Type, desc.Format, desc.MultiSampleType, desc.MultiSampleQuality);
		if (desc.Width != targetWidth || desc.Height != targetHeight) {
			DbgPrintf("[D3D9Hook] desc size not match");
		} else if (SUCCEEDED(gD3dDevice9->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &gOffscreenSurface, nullptr))) {
			// 2. 根据desc创建一个处于内存中的OffscreenPlainSurface，用于将swapChain中的内容从内存中转移出来存下
			DbgPrintf("[D3D9Hook] CreateOffscreenPlainSurface created: 0x%X", gOffscreenSurface);
			isCapturePrepared = true;
		}
		backBuffer->Release();
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
	gD3dDevice9 = nullptr;
	gD3dSwapChain = nullptr;
	isCapturePrepared = false;

	IDirect3D9Ex *pD3D9;
	Direct3DCreate9Ex(D3D_SDK_VERSION, &pD3D9);

	// Hook CreateDevice
	// const UINT_PTR *pD3D9VTable = reinterpret_cast<UINT_PTR *>(*reinterpret_cast<UINT_PTR *>(pD3D9));  // 找到vTable
	// pD3D9->Release();
	// gCreateDevice = reinterpret_cast<CreateDevice>(pD3D9VTable[15]);
	// DbgPrintf("[D3D9Hook] Starting install hook: 0x%X --> 0x%X", gCreateDevice, HookedCreateDevice);
	// if (!gSwapChainHook.InstallHook(gCreateDevice, HookedCreateDevice)) {
	// 	return 2;
	// }

	// Hook CreateDeviceEx
	// const UINT_PTR *pD3D9VTable = reinterpret_cast<UINT_PTR *>(*reinterpret_cast<UINT_PTR *>(pD3D9));  // 找到vTable
	// pD3D9->Release();
	// gCreateDeviceEx = reinterpret_cast<CreateDeviceEx>(pD3D9VTable[19]);  // magic number
	// DbgPrintf("[D3D9Hook] Starting install hook: 0x%X --> 0x%X", gCreateDeviceEx, HookedCreateDeviceEx);
	// if (!gSwapChainHook.InstallHook(gCreateDeviceEx, HookedCreateDeviceEx)) {
	// 	return 2;
	// }

	// Hook CreateAdditionalSwapChain
	IDirect3DDevice9 *pD3D9Device;
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hwnd;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	TRY(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, &d3dpp, &pD3D9Device));
	const UINT_PTR *pD3DDeviceVTable = reinterpret_cast<UINT_PTR *>(*reinterpret_cast<UINT_PTR *>(pD3D9Device));  // 找到vTable
	pD3D9Device->Release();
	pD3D9->Release();
	gCreateAdditionalSwapChain = reinterpret_cast<CreateAdditionalSwapChain>(pD3DDeviceVTable[13]);  // magic number
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