// ==WindhawkMod==
// @id              lasso-uwp-taskbar-buttons
// @name            LassoWindows UWP taskbar buttons
// @description     Hide Windows 11 taskbar buttons for UWP windows sealed by LassoWindows
// @version         1.0
// @author          LassoWindows
// @github          https://github.com/rarnhe/LassoWindows
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lole32 -lruntimeobject
// ==/WindhawkMod==

#include <windows.h>
#include <windhawk_utils.h>

#undef GetCurrentTime

#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Automation.h>
#include <winrt/Windows.UI.Xaml.Media.h>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

using namespace winrt::Windows::UI::Xaml;

namespace {
constexpr wchar_t kPublisherClass[] = L"LassoWindowsCoordinator";
constexpr wchar_t kPropertyPrefix[] = L"LassoWindows.TaskbarHide.";
constexpr wchar_t kAutomationPrefix[] = L"Appid: ";
constexpr wchar_t kTimerClass[] = L"LassoWindows.Windhawk.TaskbarTimer";
constexpr UINT_PTR kRefreshTimer = 1;

struct HiddenButton {
    winrt::weak_ref<FrameworkElement> button;
    winrt::Windows::Foundation::IInspectable originalValue;
};

struct TaskbarThread {
    HWND timerWindow = nullptr;
    std::vector<winrt::weak_ref<XamlRoot>> roots;
    std::vector<HiddenButton> hiddenButtons;
};

thread_local TaskbarThread g_thread;
std::mutex g_timerMutex;
std::vector<HWND> g_timerWindows;
std::atomic<bool> g_unloading{false};
std::atomic<bool> g_hooked{false};
HMODULE g_module = nullptr;

bool ShouldHide(const FrameworkElement& button, HWND publisher) {
    if (!publisher) return false;
    std::wstring id(Automation::AutomationProperties::GetAutomationId(button).c_str());
    if (!id.starts_with(kAutomationPrefix)) return false;
    id.erase(0, std::size(kAutomationPrefix) - 1);
    if (id.empty() || id.size() > 128) return false;
    const auto property = kPropertyPrefix + id;
    return GetPropW(publisher, property.c_str()) != nullptr;
}

void RestoreButton(const FrameworkElement& button,
                   const winrt::Windows::Foundation::IInspectable& saved) {
    const auto property = UIElement::VisibilityProperty();
    if (saved == DependencyProperty::UnsetValue())
        button.ClearValue(property);
    else
        button.SetValue(property, saved);
}

void UpdateButton(const FrameworkElement& button, HWND publisher) {
    auto& hidden = g_thread.hiddenButtons;
    const auto entry = std::find_if(hidden.begin(), hidden.end(), [&](const auto& item) {
        return item.button.get() == button;
    });
    if (ShouldHide(button, publisher)) {
        if (entry == hidden.end())
            hidden.push_back({winrt::make_weak(button), button.ReadLocalValue(UIElement::VisibilityProperty())});
        if (button.Visibility() != Visibility::Collapsed)
            button.Visibility(Visibility::Collapsed);
    } else if (entry != hidden.end()) {
        RestoreButton(button, entry->originalValue);
        hidden.erase(entry);
    }
}

void UpdateTree(const DependencyObject& node, HWND publisher) {
    if (!node) return;
    if (winrt::get_class_name(node) == L"Taskbar.TaskListButton") {
        if (auto button = node.try_as<FrameworkElement>()) UpdateButton(button, publisher);
        return;
    }
    const int children = Media::VisualTreeHelper::GetChildrenCount(node);
    for (int i = 0; i < children; ++i)
        UpdateTree(Media::VisualTreeHelper::GetChild(node, i), publisher);
}

LRESULT CALLBACK TimerWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_CLOSE) {
        KillTimer(hwnd, kRefreshTimer);
        for (const auto& entry : g_thread.hiddenButtons) {
            try {
                if (auto button = entry.button.get()) RestoreButton(button, entry.originalValue);
            } catch (...) {}
        }
        g_thread.hiddenButtons.clear();
        g_thread.roots.clear();
        g_thread.timerWindow = nullptr;
        DestroyWindow(hwnd);
        return 0;
    }
    if (msg == WM_TIMER && wp == kRefreshTimer) {
        try {
            const HWND publisher = FindWindowExW(HWND_MESSAGE, nullptr, kPublisherClass, nullptr);
            if (publisher) {
                std::erase_if(g_thread.roots, [](const auto& root) { return !root.get(); });
                for (const auto& item : g_thread.roots) {
                    if (auto root = item.get()) UpdateTree(root.Content(), publisher);
                }
            }
            for (size_t i = g_thread.hiddenButtons.size(); i > 0;) {
                const auto entry = g_thread.hiddenButtons[--i].button.get();
                if (!entry) {
                    g_thread.hiddenButtons.erase(g_thread.hiddenButtons.begin() + i);
                } else {
                    UpdateButton(entry, publisher);
                }
            }
        } catch (...) {
            Wh_Log(L"LassoWindows couldn't refresh the taskbar visual tree");
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool TrackRoot(const FrameworkElement& element) {
    auto root = element.XamlRoot();
    if (!root) return false;
    if (!g_thread.timerWindow) {
        std::lock_guard lock(g_timerMutex);
        if (g_unloading) return false;
        HWND window = CreateWindowExW(0, kTimerClass, L"", 0, 0, 0, 0, 0,
            HWND_MESSAGE, nullptr, g_module, nullptr);
        if (!window) return false;
        if (!SetTimer(window, kRefreshTimer, 200, nullptr)) {
            DestroyWindow(window);
            return false;
        }
        g_thread.timerWindow = window;
        g_timerWindows.push_back(window);
    }
    for (const auto& known : g_thread.roots) {
        if (known.get() == root) return true;
    }
    g_thread.roots.push_back(winrt::make_weak(root));
    return true;
}

using UpdateVisualStatesFn = void(WINAPI*)(void*);
UpdateVisualStatesFn g_originalUpdate = nullptr;

// The Taskbar Labels mod documents the XAML interface offset for this
// Windows 11 x64 implementation method. Resolve the named symbol first.
void WINAPI UpdateVisualStates(void* self) {
    g_originalUpdate(self);
    if (g_unloading) return;
    try {
        winrt::Windows::Foundation::IUnknown inspectable;
        winrt::copy_from_abi(inspectable, static_cast<void**>(self) + 3);
        auto button = inspectable.try_as<FrameworkElement>();
        if (button && TrackRoot(button))
            UpdateButton(button, FindWindowExW(HWND_MESSAGE, nullptr, kPublisherClass, nullptr));
    } catch (...) {
        Wh_Log(L"LassoWindows couldn't attach to a taskbar button");
    }
}

HMODULE FindTaskbarView() {
    if (HMODULE view = GetModuleHandleW(L"Taskbar.View.dll")) return view;
    return GetModuleHandleW(L"ExplorerExtensions.dll");
}

bool HookTaskbarView(HMODULE view) {
    if (g_hooked.exchange(true)) return true;
    WindhawkUtils::SYMBOL_HOOK hooks[] = {
        {{LR"(private: void __cdecl winrt::Taskbar::implementation::TaskListButton::UpdateVisualStates(void))"},
         &g_originalUpdate, UpdateVisualStates}
    };
    if (WindhawkUtils::HookSymbols(view, hooks, ARRAYSIZE(hooks))) return true;
    g_hooked = false;
    return false;
}

decltype(&LoadLibraryExW) g_originalLoadLibrary = nullptr;
HMODULE WINAPI LoadLibraryHook(LPCWSTR path, HANDLE file, DWORD flags) {
    HMODULE result = g_originalLoadLibrary(path, file, flags);
    if (!g_hooked && !g_unloading && result && result == FindTaskbarView() && HookTaskbarView(result))
        Wh_ApplyHookOperations();
    return result;
}
} // namespace

BOOL Wh_ModInit() {
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&TimerWndProc), &g_module);
    WNDCLASSW wc{};
    wc.hInstance = g_module;
    wc.lpfnWndProc = TimerWndProc;
    wc.lpszClassName = kTimerClass;
    if (!g_module || !RegisterClassW(&wc)) return FALSE;

    bool installed = false;
    if (HMODULE view = FindTaskbarView()) {
        installed = HookTaskbarView(view);
    } else {
        const auto load = reinterpret_cast<decltype(&LoadLibraryExW)>(
            GetProcAddress(GetModuleHandleW(L"kernelbase.dll"), "LoadLibraryExW"));
        installed = load && WindhawkUtils::SetFunctionHook(load, LoadLibraryHook, &g_originalLoadLibrary);
    }
    if (!installed) UnregisterClassW(kTimerClass, g_module);
    return installed;
}

void Wh_ModAfterInit() {
    if (HMODULE view = FindTaskbarView(); view && !g_hooked && HookTaskbarView(view))
        Wh_ApplyHookOperations();
    EnumWindows([](HWND window, LPARAM) -> BOOL {
        wchar_t name[32]{};
        if (GetClassNameW(window, name, ARRAYSIZE(name)) &&
            (wcscmp(name, L"Shell_TrayWnd") == 0 || wcscmp(name, L"Shell_SecondaryTrayWnd") == 0))
            PostMessageW(window, WM_SETTINGCHANGE, 0, 0);
        return TRUE;
    }, 0);
}

void Wh_ModBeforeUninit() {
    g_unloading = true;
    std::vector<HWND> windows;
    {
        std::lock_guard lock(g_timerMutex);
        windows.swap(g_timerWindows);
    }
    for (HWND window : windows) SendMessageW(window, WM_CLOSE, 0, 0);
    UnregisterClassW(kTimerClass, g_module);
}
