#include <iostream>
#include <stdexcept>
// Compile the actual implementation; console main never calls WinMain or installs hooks.
#include "../LassoWindows.cpp"
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while(false)

struct Window {
    HWND h;
    explicit Window(DWORD ex = WS_EX_APPWINDOW, HWND parent = nullptr) {
        h = CreateWindowExW(ex, L"STATIC", L"Lasso fixture",
            parent ? WS_CHILD : WS_OVERLAPPEDWINDOW, 50, 50, 240, 160,
            parent, nullptr, GetModuleHandleW(nullptr), nullptr);
        CHECK(h);
    }
    ~Window() { DestroyWindow(h); }
    operator HWND() const { return h; }
    Window(const Window&) = delete;
};
struct Ini {
    std::filesystem::path path;
    Ini() {
        wchar_t exe[MAX_PATH]{};
        CHECK(GetModuleFileNameW(nullptr, exe, MAX_PATH));
        path = std::filesystem::path(exe).parent_path() / L"desktop.ini";
        CHECK(!std::filesystem::exists(path)); // Never overwrite a user's file.
    }
    void write(const std::string& text) {
        std::ofstream file(path, std::ios::binary);
        file << text; CHECK(file.good());
    }
    ~Ini() { std::error_code ec; std::filesystem::remove(path, ec); }
};
void iniTest(const std::string& name) {
    Ini ini;
    std::wstring expected;
    int expectedIndex = 0;
    bool result = true;
    if (name == "ini_missing") result = false;
    else if (name == "ini_no_key") { ini.write("[.ShellClassInfo]\r\nOther=value\r\n"); result = false; }
    else if (name == "ini_index") { ini.write("[.ShellClassInfo]\r\nIconResource=C:\\icons.dll,12\r\n"); expected = L"C:\\icons.dll"; expectedIndex = 12; }
    else if (name == "ini_negative") { ini.write("IconResource=icons.dll,-42\n"); expected = L"icons.dll"; expectedIndex = -42; }
    else if (name == "ini_no_index") { ini.write("IconResource=icon.ico\n"); expected = L"icon.ico"; }
    else if (name == "ini_invalid") { ini.write("IconResource=icons.dll,invalid\n"); expected = L"icons.dll"; }
    else if (name == "ini_empty") { ini.write("IconResource=,3\n"); result = false; expectedIndex = 3; }
    else if (name == "ini_comma") { ini.write("IconResource=C:\\a,b\\icons.dll,7\n"); expected = L"C:\\a,b\\icons.dll"; expectedIndex = 7; }
    else throw std::runtime_error("unknown INI case");
    std::wstring actual = L"stale"; int index = 99;
    CHECK(GetIconResourceFromIni(actual, index) == result);
    CHECK(actual == expected); CHECK(index == expectedIndex);
}
void run(const std::string& name) {
    if (name.rfind("ini_", 0) == 0) { iniTest(name); return; }
    if (name == "invalid_process") {
        CHECK(GetProcessNameByPid(0).empty());
        CHECK(GetRealProcessName(nullptr) == L"Unknown"); return;
    }
    Window w;
    if (name == "process_names") {
        wchar_t exe[MAX_PATH]{};
        CHECK(GetModuleFileNameW(nullptr, exe, MAX_PATH));
        auto expected = std::filesystem::path(exe).filename().wstring();
        CHECK(GetProcessNameByPid(GetCurrentProcessId()) == expected);
        CHECK(GetRealProcessName(w) == expected);
    } else if (name == "window_filter") {
        CHECK(IsValidAppWindow(w));
        Window child(0, w), tool(WS_EX_TOOLWINDOW);
        CHECK(!IsValidAppWindow(child)); CHECK(!IsValidAppWindow(tool));
    } else if (name == "rectangle") {
        RECT actual{}, expected{};
        CHECK(!GetPhysicalRect(nullptr, &actual));
        CHECK(!GetPhysicalRect(w, nullptr));
        g_bSystemHasDpiScaling = false;
        CHECK(GetPhysicalRect(w, &actual)); CHECK(GetWindowRect(w, &expected));
        CHECK(EqualRect(&actual, &expected));
        g_bSystemHasDpiScaling = true;
        CHECK(GetPhysicalRect(w, &actual));
        CHECK(actual.right > actual.left && actual.bottom > actual.top);
    } else if (name == "visibility") {
        CHECK(!IsHiddenWindowReallyVisiable(nullptr));
        CHECK(!IsHiddenWindowReallyVisiable(w));
        ShowWindow(w, SW_SHOWNOACTIVATE); CHECK(IsHiddenWindowReallyVisiable(w));
        ShowWindow(w, SW_HIDE); CHECK(!IsHiddenWindowReallyVisiable(w));
    } else if (name == "hide_restore") {
        ShowWindow(w, SW_SHOWNOACTIVATE);
        HideWindowAndHideTaskbarIcon(w);
        auto style = GetWindowLongPtrW(w, GWL_EXSTYLE);
        auto mask = WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT;
        CHECK((style & mask) == mask); CHECK(!(style & WS_EX_APPWINDOW));
        BYTE alpha = 255; COLORREF color{}; DWORD flags{};
        CHECK(GetLayeredWindowAttributes(w, &color, &alpha, &flags));
        CHECK(alpha == 0 && (flags & LWA_ALPHA));
        ShowWindowAndShowTaskbarIcon(w);
        style = GetWindowLongPtrW(w, GWL_EXSTYLE);
        CHECK(style & WS_EX_APPWINDOW); CHECK(!(style & mask));
        CHECK(IsWindowVisible(w));
        HideWindowAndHideTaskbarIcon(nullptr); ShowWindowAndShowTaskbarIcon(nullptr);
    } else if (name == "restore_tool_style") {
        ShowWindow(w, SW_SHOWNOACTIVATE);
        const LONG_PTR original = GetWindowLongPtrW(w, GWL_EXSTYLE);
        SetWindowLongPtrW(w, GWL_EXSTYLE, (original & ~WS_EX_APPWINDOW) | WS_EX_TOOLWINDOW);
        const LONG_PTR tool = GetWindowLongPtrW(w, GWL_EXSTYLE);
        HideWindowAndHideTaskbarIcon(w);
        ShowWindowAndShowTaskbarIcon(w);
        CHECK(GetWindowLongPtrW(w, GWL_EXSTYLE) == tool);
        CHECK(!g_sealedWindowStates.contains(w.h));
    } else if (name == "restore_layered_alpha") {
        ShowWindow(w, SW_SHOWNOACTIVATE);
        LONG_PTR style = GetWindowLongPtrW(w, GWL_EXSTYLE);
        SetWindowLongPtrW(w, GWL_EXSTYLE, style | WS_EX_LAYERED);
        CHECK(SetLayeredWindowAttributes(w, RGB(13, 42, 71), 173, LWA_ALPHA));
        const LONG_PTR initial = GetWindowLongPtrW(w, GWL_EXSTYLE);
        HideWindowAndHideTaskbarIcon(w);
        ShowWindowAndShowTaskbarIcon(w);
        BYTE alpha = 0; COLORREF color{}; DWORD flags = 0;
        CHECK(GetWindowLongPtrW(w, GWL_EXSTYLE) == initial);
        CHECK(GetLayeredWindowAttributes(w, &color, &alpha, &flags));
        CHECK(alpha == 173 && (flags & LWA_ALPHA));
        CHECK(!g_sealedWindowStates.contains(w.h));
    } else if (name == "preview_preserves_snapshot") {
        ShowWindow(w, SW_SHOWNOACTIVATE);
        const LONG_PTR initial = GetWindowLongPtrW(w, GWL_EXSTYLE);
        HideWindowAndHideTaskbarIcon(w);
        CHECK(g_sealedWindowStates.contains(w.h));
        ShowWindowAndShowTaskbarIcon(w, true);
        CHECK(GetWindowLongPtrW(w, GWL_EXSTYLE) == initial);
        CHECK(g_sealedWindowStates.contains(w.h));
        HideWindowAndHideTaskbarIcon(w);
        ShowWindowAndShowTaskbarIcon(w);
        CHECK(GetWindowLongPtrW(w, GWL_EXSTYLE) == initial);
        CHECK(!g_sealedWindowStates.contains(w.h));
    } else if (name == "hide_others_context") {
        CloseFilter filter;
        filter.currentAppProcName = L"nonexistent_test_process.exe";
        ShowWindow(w, SW_SHOWNOACTIVATE);
        CHECK(EnumHideOthersProc(w, reinterpret_cast<LPARAM>(&filter)));
        if (g_sealedWindowStates.contains(w.h)) ShowWindowAndShowTaskbarIcon(w);
    } else if (name == "untracked_restore") {
        const LONG_PTR original = GetWindowLongPtrW(w, GWL_EXSTYLE);
        ShowWindowAndShowTaskbarIcon(w);
        CHECK(GetWindowLongPtrW(w, GWL_EXSTYLE) == original);
    } else if (name == "suppress") {
        CHECK(SetPropW(w, L"MTU_PROC_TAG", reinterpret_cast<HANDLE>(1)));
        g_InSnapWindows = true;
        SuppressHiddenWindows(w, EVENT_OBJECT_SHOW);
        CHECK(!(GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW));
        g_InSnapWindows = false;
        SuppressHiddenWindows(w, EVENT_OBJECT_FOCUS);
        CHECK(!(GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW));
        SuppressHiddenWindows(w, EVENT_OBJECT_SHOW);
        CHECK(GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW);
        RemovePropW(w, L"MTU_PROC_TAG");
        ShowWindowAndShowTaskbarIcon(w);
        SuppressHiddenWindows(w, EVENT_OBJECT_SHOW);
        CHECK(!(GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW));
    } else if (name == "collect") {
        g_InSnapWindows = true; g_AltLogicActive = true;
        g_snapUnhiddenWindows = {w, nullptr};
        ExecuteCollectRestore();
        CHECK(g_snapUnhiddenWindows.empty());
        CHECK(!g_InSnapWindows && !g_AltLogicActive);
        CHECK(GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW);
        ExecuteCollectRestore(); CHECK(g_snapUnhiddenWindows.empty());
    } else if (name == "icon_cache") {
        auto icon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512)); CHECK(icon);
        SendMessageW(w, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
        bool extracted = true;
        CHECK(GetOriginalWindowIcon(w, extracted) == icon); CHECK(!extracted);
        CHECK(GetPropW(w, L"MTU_ORIGINAL_ICON_KEEP") == icon);
        SendMessageW(w, WM_SETICON, ICON_BIG, 0);
        CHECK(GetOriginalWindowIcon(w, extracted) == icon); CHECK(!extracted);
        RemovePropW(w, L"MTU_ORIGINAL_ICON_KEEP"); RemovePropW(w, L"MTU_ICON_NEED_DESTROY");
    } else if (name == "toggle_filter") {
        std::vector<HWND> found;
        ToggleContext context{&found, MonitorFromWindow(w, MONITOR_DEFAULTTOPRIMARY)};
        CHECK(EnumWindowToggleProc(w, 0));
        CHECK(EnumWindowToggleProc(w, reinterpret_cast<LPARAM>(&context)));
        CHECK(found.empty());
        ShowWindow(w, SW_SHOWNOACTIVATE);
        CHECK(SetPropW(w, L"MTU_PROC_TAG", reinterpret_cast<HANDLE>(1)));
        CHECK(EnumWindowToggleProc(w, reinterpret_cast<LPARAM>(&context)));
        CHECK(found.empty());
        RemovePropW(w, L"MTU_PROC_TAG");
        CHECK(EnumWindowToggleProc(w, reinterpret_cast<LPARAM>(&context)));
        CHECK(found.size() == 1 && found.front() == w.h);
    } else throw std::runtime_error("unknown case");
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try { run(argv[1]); std::cout << "PASS: " << argv[1] << '\n'; return 0; }
    catch (const std::exception& e) { std::cerr << "FAIL: " << argv[1] << ": " << e.what() << '\n'; return 1; }
}
