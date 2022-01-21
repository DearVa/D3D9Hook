#include "HookJump.h"
#include "Dll.h"
#include <d3d9helper.h>
#include <d3dx9tex.h>
#include <cstdio>
#include <mutex>

#define TRY(func) do { auto hr = func; if (FAILED(hr)) return hr; } while(0)

static CHookJump gSwapChainHook;
static HWND targetHwnd;
static std::mutex mutex;

static SWAPCHAIN_PRESENT gSwapChainPresent;
static IDirect3DSwapChain9 *gSwapChain9;
static IDirect3DDevice9 *gD3dDevice9 = nullptr;

static D3DSURFACE_DESC desc;
static LPDIRECT3DSURFACE9 pd3dsBack;
static LPDIRECT3DSURFACE9 pd3dsTemp;

static void *pixelBuf;

/**
 * \brief 将device内容拷贝到buf内
 * \param width
 * \param height
 * \param buf
 * \return
 */
extern "C" __declspec(dllexport) int WINAPI Capture(UINT * width, UINT * height, void **buf) {
	if (gD3dDevice9 == nullptr) {
		return 1;
	}
	if (FAILED(gD3dDevice9->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &pd3dsTemp, NULL))) {
		return 2;
	}
	if (FAILED(gD3dDevice9->GetRenderTargetData(pd3dsBack, pd3dsTemp))) {
		return 3;
	}
	D3DLOCKED_RECT lockedSrcRect;
	if (FAILED(pd3dsTemp->LockRect(&lockedSrcRect, NULL, D3DLOCK_READONLY | D3DLOCK_NOSYSLOCK | D3DLOCK_NO_DIRTY_UPDATE))) {
		return 4;
	}
	*width = desc.Width;
	*height = desc.Height;
	memcpy(pixelBuf, lockedSrcRect.pBits, static_cast<size_t>(desc.Width) * desc.Height * 4);
	*buf = pixelBuf;
	pd3dsTemp->UnlockRect();
	return 0;
}

extern "C" __declspec(dllexport) bool WINAPI IsCaptureReady() {
	DbgPrintf("[D3D9Hook] gD3dDevice9: 0x%X", gD3dDevice9);
	return gD3dDevice9 != nullptr;
}

/**
 * \brief 释放所有的资源（如果有）
 */
void Release() {
	if (pd3dsTemp != nullptr) {
		pd3dsTemp->Release();
	}
	if (pd3dsBack != nullptr) {
		pd3dsBack->Release();
	}
	if (gD3dDevice9 != nullptr) {
		gD3dDevice9->Release();
	}
	if (pixelBuf != nullptr) {
		free(pixelBuf);
	}
}

/**
 * \brief 为gSwapChain9创建RenderTarget
 */
void PrepareForCapture(IDirect3DDevice9 *device) {
	Release();

	// Grab the back buffer into a surface
	if (SUCCEEDED(gSwapChain9->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pd3dsBack))) {
		pd3dsBack->GetDesc(&desc);
		LPDIRECT3DSURFACE9 pd3dsCopy = nullptr;
		DbgPrintf("[D3D9Hook] Got desc, width: %d, height: %d, multiSampleType: %d", desc.Width, desc.Height, desc.MultiSampleType);
		if (SUCCEEDED(device->CreateRenderTarget(desc.Width, desc.Height, desc.Format, D3DMULTISAMPLE_NONE, 0, FALSE, &pd3dsCopy, NULL))) {
			DbgPrintf("[D3D9Hook] RenderTarget created: 0x%X", pd3dsCopy);
			if (SUCCEEDED(device->StretchRect(pd3dsBack, NULL, pd3dsCopy, NULL, D3DTEXF_NONE))) {
				pd3dsBack->Release();
				pd3dsBack = pd3dsCopy;
				if (pixelBuf == nullptr) {
					pixelBuf = malloc(static_cast<size_t>(desc.Width) * desc.Height * 4);
				} else {
					pixelBuf = realloc(pixelBuf, static_cast<size_t>(desc.Width) * desc.Height * 4);
				}
				gD3dDevice9 = device;  // 初始化完毕
				DbgPrintf("[D3D9Hook] RenderTarget successfully created. gD3dDevice9: 0x%X", gD3dDevice9);
			} else {
				pd3dsCopy->Release();
			}
		}
	}
}

//#include <unordered_set>
//std::unordered_set<IDirect3DSwapChain9 *> swapChains;

HRESULT WINAPI SwapChainPresentHooked(void *pSwapChain, RECT *pSourceRect, RECT *pDestRect, const HWND hDestWindowOverride, RGNDATA *pDirtyRegion, const DWORD dwFlags) {
	mutex.lock();
	gSwapChainHook.SwapOld(gSwapChainPresent);

	// 这样就拿到SwapChain了
	const auto swapChain = static_cast<IDirect3DSwapChain9 *>(pSwapChain);
	D3DPRESENT_PARAMETERS dp;
	if (SUCCEEDED(swapChain->GetPresentParameters(&dp))) {
		DbgPrintf("[D3D9Hook] Hooked SwapChainPresent: 0x%X, hDeviceWindow: 0x%X, targetHwnd: 0x%X", pSwapChain, dp.hDeviceWindow, targetHwnd);
		if (dp.hDeviceWindow == targetHwnd) {
			gSwapChain9 = swapChain;
			DbgPrintf("[D3D9Hook] Got gSwapChain9: 0x%X", gSwapChain9);
			IDirect3DDevice9 *d3dDevice;
			auto const hr = swapChain->GetDevice(&d3dDevice);
			if (FAILED(hr)) {
				DbgPrintf("[D3D9Hook] GetDevice error: 0x%X", hr);
			} else if (d3dDevice != gD3dDevice9) {  // 新的device和旧的不一样
				DbgPrintf("[D3D9Hook] Preparing for capture: 0x%X", d3dDevice);
				PrepareForCapture(d3dDevice);
			}
		}
	}

	const auto hr = gSwapChainPresent(pSwapChain, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
	gSwapChainHook.SwapReset(gSwapChainPresent);
	mutex.unlock();
	// 调用原始函数并返回
	return hr;
}

extern "C" __declspec(dllexport) bool WINAPI Unhook() {
	Release();  // 以防万一，先尝试释放资源
	if (!gSwapChainHook.IsHookInstalled()) {
		return false;
	}
	DbgPrintf("[D3D9Hook] Starting uninstall hook: 0x%X", gSwapChainPresent);
	return gSwapChainHook.UninstallHook(gSwapChainPresent);
}

/**
 * \brief Hook，之后可以调用
 * \param hwnd DX的窗口
 * \return 成功返回0
 */
extern "C" __declspec(dllexport) HRESULT WINAPI Hook(const HWND hwnd) {
	DbgPrintf("[D3D9Hook] Starting hook, hwnd: 0x%X", hwnd);
	if (gSwapChainHook.IsHookInstalled()) {
		DbgPrintf("[D3D9Hook] Hook installed, return");
		return 1;
	}
	targetHwnd = hwnd;
	/* First we must create a D3D resources to
	 * be able to get the method pointers to hook */
	IDirect3D9Ex *pD3D9;
	IDirect3DDevice9 *pD3D9Device;
	IDirect3DSwapChain9 *pSwapChain;

	/* Nothing out of the ordinary */
	Direct3DCreate9Ex(D3D_SDK_VERSION, &pD3D9);

	/* Standard present parameters for D3D9 */
	/*D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hwnd;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;*/

	const UINT_PTR *pD3D9VTable = reinterpret_cast<UINT_PTR *>(*reinterpret_cast<UINT_PTR *>(pD3D9));


	/* Create the IDirect3DDevice9 */
	TRY(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, &d3dpp, &pD3D9Device));

	/* Create the swap chain */
	TRY(pD3D9Device->CreateAdditionalSwapChain(&d3dpp, &pSwapChain));

	/* Get the vtable for the swap chain */
	const UINT_PTR *pSwapChainVTable = reinterpret_cast<UINT_PTR *>(/*Indirection ->*/*reinterpret_cast<UINT_PTR *>(pSwapChain));

	/* The third address is the swap chain Present(...) */
	gSwapChainPresent = reinterpret_cast<SWAPCHAIN_PRESENT>(pSwapChainVTable[3]);

	gSwapChain9 = nullptr;
	/* Hook into the method */
	DbgPrintf("[D3D9Hook] Starting install hook: 0x%X --> 0x%X", gSwapChainPresent, SwapChainPresentHooked);
	if (!gSwapChainHook.InstallHook(gSwapChainPresent, SwapChainPresentHooked)) {
		return -1;
	}
	DbgPrintf("[D3D9Hook] Hook installed");

	/* Free our temporary resources */
	pSwapChain->Release();
	pD3D9Device->Release();
	pD3D9->Release();

	return S_OK;
}

BOOL APIENTRY DllMain(HMODULE hModule, const DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH) {
		DbgPrintf("[D3D9Hook] DLL_PROCESS_ATTACH: 0x%X", gD3dDevice9);
	}
	return TRUE;
}