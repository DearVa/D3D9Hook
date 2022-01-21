#pragma once
#include <Windows.h>
#include <windef.h>
#include <d3d9.h>

/* Method prototypes */
typedef HRESULT (WINAPI *SwapchainPresent)(void * pSwapChain, RECT* pSourceRect, RECT* pDestRect,HWND hDestWindowOverride, RGNDATA* pDirtyRegion, DWORD dwFlags);
typedef HRESULT (WINAPI *CreateDevice)(IDirect3D9 *This, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DDevice9 **ppReturnedDeviceInterface);
typedef HRESULT (WINAPI *CreateDeviceEx)(IDirect3D9Ex *This, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters, D3DDISPLAYMODEEX *pFullscreenDisplayMode, IDirect3DDevice9Ex **ppReturnedDeviceInterface);
typedef HRESULT (WINAPI *CreateAdditionalSwapChain)(IDirect3DDevice9 *This, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DSwapChain9 **pSwapChain);