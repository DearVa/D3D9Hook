#pragma once
#include <Windows.h>
#include <windef.h>
#include <d3d9.h>

/* Method prototypes */
typedef HRESULT (STDMETHODCALLTYPE *SWAPCHAIN_PRESENT)(void * pSwapChain, RECT* pSourceRect, RECT* pDestRect,HWND hDestWindowOverride, RGNDATA* pDirtyRegion, DWORD dwFlags);
typedef HRESULT (STDMETHODCALLTYPE *CREATE_DEVICE)(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DDevice9 **ppReturnedDeviceInterface);