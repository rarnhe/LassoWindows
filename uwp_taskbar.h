#pragma once

#include <windows.h>
#include <inspectable.h>
#include <servprov.h>
#include <shobjidl_core.h>
#include <winrt/base.h>

namespace uwp_taskbar {

// Undocumented Shell interfaces. Keep the vtable prefix intact.
// Layout: https://github.com/Ciantic/VirtualDesktopAccessor/blob/rust/src/interfaces.rs
struct __declspec(uuid("372E1D3B-38D3-42E4-A15B-8AB2B178F513"))
ApplicationView : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE SetFocus() = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchTo() = 0;
    virtual HRESULT STDMETHODCALLTYPE TryInvokeBack(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetThumbnailWindow(HWND*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMonitor(IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVisibility(int*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCloak(int, int) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPosition(REFIID, void**) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPosition(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE InsertAfterWindow(HWND) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetExtendedFramePosition(RECT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAppUserModelId(PWSTR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetAppUserModelId(PCWSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsEqualByAppUserModelId(PCWSTR, int*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewState(UINT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetViewState(UINT) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetNeediness(int*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLastActivationTimestamp(ULONGLONG*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetLastActivationTimestamp(ULONGLONG) = 0;
    // Unused slots required to preserve the Shell ABI for the switcher methods.
    virtual HRESULT STDMETHODCALLTYPE ReservedSlot19() = 0;
    virtual HRESULT STDMETHODCALLTYPE ReservedSlot20() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetShowInSwitchers(int*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetShowInSwitchers(int) = 0;
};

struct __declspec(uuid("1841C6D7-4F9D-42C0-AF41-8747538F10E5"))
ApplicationViewCollection : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetViews(IObjectArray**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewsByZOrder(IObjectArray**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewsByAppUserModelId(PCWSTR, IObjectArray**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewForHwnd(HWND, ApplicationView**) = 0;
};

inline constexpr CLSID CLSID_IMMERSIVE_SHELL{
    0xc2f03a33, 0x21f5, 0x47fa, {0xb4, 0xbb, 0x15, 0x63, 0x62, 0xa2, 0xf2, 0x39}
};
inline constexpr wchar_t PROP_RESTORE_SWITCHER[] = L"LassoWindows.UwpRestoreSwitcher";

inline HRESULT FindView(HWND hwnd, winrt::com_ptr<ApplicationView>& view) {
    winrt::com_ptr<IServiceProvider> shell;
    HRESULT hr = CoCreateInstance(CLSID_IMMERSIVE_SHELL, nullptr, CLSCTX_LOCAL_SERVER,
        __uuidof(IServiceProvider), shell.put_void());
    if (FAILED(hr)) return hr;
    if (!shell) return E_NOINTERFACE;

    winrt::com_ptr<ApplicationViewCollection> collection;
    const auto& iid = __uuidof(ApplicationViewCollection);
    hr = shell->QueryService(iid, iid, collection.put_void());
    if (FAILED(hr)) return hr;
    if (!collection) return E_NOINTERFACE;
    return collection->GetViewForHwnd(hwnd, view.put());
}

// The property belongs to the HWND so a later launch can undo an interrupted
// seal. It is only set when the view was originally enabled.
template <typename View>
HRESULT SetSwitcherVisibility(HWND hwnd, bool restore, View& view) {
    const bool owned = GetPropW(hwnd, PROP_RESTORE_SWITCHER) != nullptr;
    if (restore && !owned) return S_OK;

    int shown = 0;
    HRESULT hr = view.GetShowInSwitchers(&shown);
    if (FAILED(hr)) return hr;
    if (shown != 0 && shown != 1) return E_UNEXPECTED;

    const int requested = restore ? 1 : 0;
    if (shown != requested) {
        if (!restore && !owned &&
            !SetPropW(hwnd, PROP_RESTORE_SWITCHER, reinterpret_cast<HANDLE>(UINT_PTR{1})))
            return E_FAIL;

        const HRESULT write = view.SetShowInSwitchers(requested);
        hr = view.GetShowInSwitchers(&shown);
        if (FAILED(hr)) return FAILED(write) ? write : hr;
        if (shown != requested) {
            if (!restore && !owned && shown == 1)
                RemovePropW(hwnd, PROP_RESTORE_SWITCHER);
            return FAILED(write) ? write : E_FAIL;
        }
    }
    if (restore) RemovePropW(hwnd, PROP_RESTORE_SWITCHER);
    return S_OK;
}

inline HRESULT SetSwitcherVisibility(HWND hwnd, bool restore) {
    if (restore && !GetPropW(hwnd, PROP_RESTORE_SWITCHER)) return S_OK;
    winrt::com_ptr<ApplicationView> view;
    const HRESULT hr = FindView(hwnd, view);
    if (FAILED(hr)) return hr;
    return view ? SetSwitcherVisibility(hwnd, restore, *view.get()) : E_NOINTERFACE;
}

} // namespace uwp_taskbar
