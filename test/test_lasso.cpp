#include <iostream>
#include <stdexcept>
// Compile the actual implementation; console main never calls WinMain or installs hooks.
#include "../LassoWindows.cpp"
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while(false)

struct Window {
    HWND h;
    explicit Window(DWORD ex = WS_EX_APPWINDOW, HWND parent = nullptr, const wchar_t* windowClass = L"STATIC") {
        h = CreateWindowExW(ex, windowClass, L"Lasso fixture",
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
    if (name == "uwp_frame_close") {
        WNDCLASSW wc{};
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"ApplicationFrameWindow";
        CHECK(RegisterClassW(&wc));
        Window frame(WS_EX_APPWINDOW, nullptr, wc.lpszClassName);
        ShowWindow(frame, SW_SHOWNOACTIVATE);
        Window ordinary;
        CHECK(!RequestHostedUwpClose(ordinary));
        CHECK(!g_sealedWindowStates.contains(frame.h));
        CHECK(RequestHostedUwpClose(frame));
        CHECK(IsWindow(frame)); // PostMessage queues the request.
        MSG message{};
        CHECK(PeekMessageW(&message, frame, WM_CLOSE, WM_CLOSE, PM_REMOVE));
        CHECK(message.hwnd == frame.h && message.wParam == 0 && message.lParam == 0);
        DispatchMessageW(&message);
        CHECK(!IsWindow(frame));
        CHECK(!g_sealedWindowStates.contains(frame.h));
        return;
    }
    if (name == "exe_tag_atom") {
        Window tagged;
        static constexpr GUID desktop{0x4fd230e8, 0x260c, 0x4fa9, {0x92, 0x34, 0x15, 0x11, 0x55, 0xa7, 0xb7, 0x39}};
        wchar_t desktopText[40]{};
        CHECK(StringFromGUID2(desktop, desktopText, static_cast<int>(std::size(desktopText))));
        CHECK(SetSealTag(tagged, L"MTU_EXE_TAG", L"notepad.exe"));
        CHECK(SetSealTag(tagged, L"MTU_VD_TAG", desktopText));
        CHECK(ReadTaggedExe(tagged) == L"notepad.exe");
        GUID actual{};
        CHECK(ReadTaggedDesktopGuid(tagged, actual) && IsEqualGUID(actual, desktop));
        CHECK(GetPropW(tagged, L"MTU_EXE_TAG") && GetPropW(tagged, L"MTU_VD_TAG"));
        ClearSealMarkers(tagged);
        CHECK(!GetPropW(tagged, L"MTU_EXE_TAG") && !GetPropW(tagged, L"MTU_VD_TAG"));
        CHECK(ReadTaggedExe(tagged).empty());
        return;
    }
    if (name == "alt_tab_after_desktop_switch") {
        Window desktopA, desktopB, desktopBSibling, coordinator;
        static constexpr GUID deskA{0x83fa8748, 0xa4c9, 0x4470, {0x8c, 0x7a, 0xe5, 0x7a, 0xa3, 0x2d, 0x85, 0xf9}};
        static constexpr GUID deskB{0x83fa8748, 0xa4c9, 0x4470, {0x8c, 0x7a, 0xe5, 0x7a, 0xa3, 0x2d, 0x85, 0xfa}};
        for (HWND h : {desktopA.h, desktopB.h, desktopBSibling.h})
            ShowWindow(h, SW_SHOWNOACTIVATE);
        CHECK(SealWindow(desktopA, L"shared-fixture.exe", deskA));
        CHECK(SealWindow(desktopB, L"shared-fixture.exe", deskB));
        CHECK(SealWindow(desktopBSibling, L"shared-fixture.exe", deskB));
        CHECK(TaggedDesktopMatches(desktopA, deskA));
        CHECK(TaggedDesktopMatches(desktopB, deskB));
        CHECK(TaggedDesktopMatches(desktopBSibling, deskB));

        // Simulate the case where all same-EXE windows are sealed and the
        // foreground is the Shell: VDM cannot identify a current sealed HWND.
        static const auto readDesktop = +[](HWND, GUID&, bool) { return false; };
        static const auto readRegistry = +[](const wchar_t* key, const wchar_t* value, GUID& id) {
            if (wcscmp(value, L"CurrentVirtualDesktop") != 0) return false;
            if (wcsstr(key, L"SessionInfo\\77\\VirtualDesktops")) {
                id = deskB;
                return true;
            }
            if (_wcsicmp(key, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VirtualDesktops") == 0) {
                id = deskA; // Stale unscoped value from the desktop we left.
                return true;
            }
            return false;
        };
        auto currentDesktop = +[](GUID& id) {
            return GetCurrentDesktopGuidFromForeground(id, nullptr, readDesktop, readRegistry, 77);
        };
        GUID resolved{};
        CHECK(currentDesktop(resolved) && IsEqualGUID(resolved, deskB));

        g_hLassoCoordinator = coordinator;
        CHECK(BeginAltTabPreview(coordinator, nullptr, currentDesktop));
        CHECK(g_snapUnhiddenWindows.size() == 2);
        CHECK(std::find(g_snapUnhiddenWindows.begin(), g_snapUnhiddenWindows.end(), desktopB.h) != g_snapUnhiddenWindows.end());
        CHECK(std::find(g_snapUnhiddenWindows.begin(), g_snapUnhiddenWindows.end(), desktopBSibling.h) != g_snapUnhiddenWindows.end());
        CHECK(std::find(g_snapUnhiddenWindows.begin(), g_snapUnhiddenWindows.end(), desktopA.h) == g_snapUnhiddenWindows.end());
        CHECK(GetWindowLongPtrW(desktopA, GWL_EXSTYLE) & WS_EX_TOOLWINDOW);
        CHECK(!(GetWindowLongPtrW(desktopB, GWL_EXSTYLE) & WS_EX_TOOLWINDOW));

        FinishAltTabPreview(desktopB, currentDesktop);
        CHECK(!g_InSnapWindows && g_snapUnhiddenWindows.empty());
        CHECK(GetPropW(desktopA, L"MTU_EXE_TAG") && GetPropW(desktopA, L"MTU_VD_TAG"));
        CHECK(g_sealedWindowStates.contains(desktopA.h));
        for (HWND h : {desktopB.h, desktopBSibling.h}) {
            CHECK(!GetPropW(h, L"MTU_EXE_TAG") && !GetPropW(h, L"MTU_VD_TAG"));
            CHECK(!g_sealedWindowStates.contains(h));
            CHECK(IsWindowVisible(h));
        }
        g_hLassoCoordinator = nullptr;
        RestoreAllSealedWindows();
        return;
    }
    if (name == "alt_tab_consecutive_previews") {
        Window preview(0), otherApp, coordinator;
        ShowWindow(preview, SW_SHOWNOACTIVATE);
        const LONG_PTR originalStyle = GetWindowLongPtrW(preview, GWL_EXSTYLE);
        CHECK(!(originalStyle & WS_EX_APPWINDOW));
        ShowWindow(otherApp, SW_SHOWNOACTIVATE);
        const GUID desktop{1};
        CHECK(SealWindow(preview, L"sealed-fixture.exe", desktop));
        const HANDLE procTag = GetPropW(preview, L"MTU_EXE_TAG");
        const HANDLE desktopTag = GetPropW(preview, L"MTU_VD_TAG");
        CHECK(procTag && desktopTag);
        auto currentDesktop = +[](GUID& id) { id = GUID{1}; return true; };
        g_hLassoCoordinator = coordinator;
        auto checkSealed = [&] {
            CHECK(GetPropW(preview, L"MTU_EXE_TAG") == procTag);
            CHECK(GetPropW(preview, L"MTU_VD_TAG") == desktopTag);
            CHECK(ReadTaggedExe(preview) == L"sealed-fixture.exe");
            CHECK(TaggedDesktopMatches(preview, desktop));
            CHECK(g_sealedWindowStates.contains(preview.h));
            CHECK(IsWindowVisible(preview));
            CHECK(GetWindowLongPtrW(preview, GWL_EXSTYLE) & WS_EX_TOOLWINDOW);
            BYTE alpha = 255;
            DWORD flags = 0;
            CHECK(GetLayeredWindowAttributes(preview, nullptr, &alpha, &flags));
            CHECK((flags & LWA_ALPHA) && alpha == 0);
        };

        CHECK(BeginAltTabPreview(coordinator, otherApp, currentDesktop));
        CHECK(g_snapUnhiddenWindows == std::vector<HWND>{preview.h});
        CHECK(IsWindowVisible(preview));
        CHECK(GetWindowLongPtrW(preview, GWL_EXSTYLE) == (originalStyle | WS_EX_APPWINDOW));
        FinishAltTabPreview(otherApp, currentDesktop);
        CHECK(!g_InSnapWindows);
        checkSealed();

        CHECK(BeginAltTabPreview(coordinator, otherApp, currentDesktop));
        CHECK(!(GetWindowLongPtrW(preview, GWL_EXSTYLE) & WS_EX_TOOLWINDOW));
        ScheduleAltTabCollect(coordinator);
        CHECK(g_altTabNextCollectAt);
        // The next chord arrives before the first release's queued timer.
        CHECK(BeginAltTabPreview(coordinator, otherApp, currentDesktop));
        CHECK(!g_altTabNextCollectAt);
        OnCollectTimer(coordinator, TIMER_ID_COLLECT);
        CHECK(g_InSnapWindows && g_snapUnhiddenWindows == std::vector<HWND>{preview.h});
        CHECK(IsWindowVisible(preview));
        CHECK(GetWindowLongPtrW(preview, GWL_EXSTYLE) == (originalStyle | WS_EX_APPWINDOW));

        ExecuteCollectRestore();
        CHECK(!g_InSnapWindows);
        checkSealed();

        CHECK(BeginAltTabPreview(coordinator, otherApp, currentDesktop));
        FinishAltTabPreview(preview, currentDesktop);
        CHECK(!g_InSnapWindows && IsWindowVisible(preview));
        CHECK(GetWindowLongPtrW(preview, GWL_EXSTYLE) == originalStyle);
        CHECK(!GetPropW(preview, L"MTU_EXE_TAG") && !GetPropW(preview, L"MTU_VD_TAG"));
        CHECK(!g_sealedWindowStates.contains(preview.h));
        g_hLassoCoordinator = nullptr;
        return;
    }
    if (name == "cross_desktop_win_c") {
        Window current, selected, otherDesktop, otherDesktopSibling, inactive, unknownDesktop;
        static constexpr GUID deskA{0x83fa8748, 0xa4c9, 0x4470, {0x8c, 0x7a, 0xe5, 0x7a, 0xa3, 0x2d, 0x85, 0xf9}};
        static constexpr GUID deskB{0x83fa8748, 0xa4c9, 0x4470, {0x8c, 0x7a, 0xe5, 0x7a, 0xa3, 0x2d, 0x85, 0xfa}};
        static GUID activeDesktop = deskA;
        static const auto readDesktop = +[](HWND h, GUID& id, bool requireCurrent) {
            if (GetPropW(h, L"Fixture.UnknownDesktop")) return false;
            id = GetPropW(h, L"Fixture.SecondDesktop") ? deskB : deskA;
            return !requireCurrent || IsEqualGUID(id, activeDesktop);
        };
        for (HWND h : {current.h, selected.h, otherDesktop.h, otherDesktopSibling.h, unknownDesktop.h})
            ShowWindow(h, SW_SHOWNOACTIVATE);
        for (HWND h : {otherDesktop.h, otherDesktopSibling.h})
            CHECK(SetPropW(h, L"Fixture.SecondDesktop", reinterpret_cast<HANDLE>(UINT_PTR{1})));
        CHECK(SetPropW(unknownDesktop, L"Fixture.UnknownDesktop", reinterpret_cast<HANDLE>(UINT_PTR{1})));

        const auto process = GetRealProcessName(current);
        CHECK(IsIdentifiedProcess(process));
        // Exercise the full Win+C entry point, including EnumWindows, with simulated desktop membership.
        HideCurrentApplication(current, readDesktop, +[](HWND h) {
            GUID desktop{};
            return readDesktop(h, desktop, true);
        });
        auto checkSealed = [&](HWND h, REFGUID desktop) {
            CHECK(ReadTaggedExe(h) == process && TaggedDesktopMatches(h, desktop));
            CHECK(GetWindowLongPtrW(h, GWL_EXSTYLE) & WS_EX_TOOLWINDOW);
            BYTE alpha = 255;
            DWORD flags = 0;
            CHECK(GetLayeredWindowAttributes(h, nullptr, &alpha, &flags));
            CHECK(alpha == 0 && (flags & LWA_ALPHA));
        };
        checkSealed(current, deskA);
        checkSealed(selected, deskA);
        checkSealed(otherDesktop, deskB);
        checkSealed(otherDesktopSibling, deskB);
        for (HWND h : {inactive.h, unknownDesktop.h})
            CHECK(!GetPropW(h, L"MTU_EXE_TAG") && !GetPropW(h, L"MTU_VD_TAG"));

        Window otherProcess, invalidProcess, oldDesktopName;
        for (HWND h : {otherProcess.h, invalidProcess.h, oldDesktopName.h})
            ShowWindow(h, SW_SHOWNOACTIVATE);
        CHECK(SealWindow(otherProcess, L"other-fixture.exe", deskA));
        CHECK(SealWindow(invalidProcess, L"invalid-fixture.exe", deskA));
        CHECK(SealWindow(oldDesktopName, L"legacy-fixture.exe", deskA));
        CHECK(SetSealTag(invalidProcess, L"MTU_EXE_TAG", L"Unknown"));
        CHECK(SetSealTag(oldDesktopName, L"MTU_VD_TAG", L"Fixture desktop name"));
        static int desktopReads = 0;
        auto firstDesktop = +[](GUID& id) {
            ++desktopReads;
            activeDesktop = deskA;
            return GetCurrentDesktopGuidFromForeground(id, nullptr, readDesktop) && IsEqualGUID(id, deskA);
        };
        auto secondDesktop = +[](GUID& id) {
            activeDesktop = deskB;
            return GetCurrentDesktopGuidFromForeground(id, nullptr, readDesktop) && IsEqualGUID(id, deskB);
        };
        CHECK(!TryUnHideWindowsForAltTab(+[](GUID&) { return false; }));
        CHECK(TryUnHideWindowsForAltTab(firstDesktop));
        CHECK(desktopReads == 1);
        CHECK(TryUnHideWindowsForAltTab(firstDesktop)); // Repeated tabs reuse the preview.
        CHECK(desktopReads == 1);
        CHECK(g_snapUnhiddenWindows.size() == 3);
        CHECK(!(GetWindowLongPtrW(selected, GWL_EXSTYLE) & WS_EX_TOOLWINDOW));
        CHECK(!(GetWindowLongPtrW(otherProcess, GWL_EXSTYLE) & WS_EX_TOOLWINDOW));
        CHECK(GetWindowLongPtrW(invalidProcess, GWL_EXSTYLE) & WS_EX_TOOLWINDOW);
        CHECK(GetWindowLongPtrW(oldDesktopName, GWL_EXSTYLE) & WS_EX_TOOLWINDOW);
        CHECK(GetWindowLongPtrW(otherDesktop, GWL_EXSTYLE) & WS_EX_TOOLWINDOW);

        WinEventProc(nullptr, EVENT_SYSTEM_FOREGROUND, selected, OBJID_WINDOW, CHILDID_SELF, 0, 0);
        CHECK(g_InSnapWindows);
        FinishAltTabPreview(selected, firstDesktop);
        CHECK(desktopReads == 2);
        CHECK(!g_InSnapWindows && g_snapUnhiddenWindows.empty());
        CHECK(IsWindowVisible(selected) && IsWindowVisible(current));
        CHECK(ReadTaggedExe(selected).empty() && !GetPropW(selected, L"MTU_VD_TAG"));
        CHECK(ReadTaggedExe(current).empty() && !GetPropW(current, L"MTU_VD_TAG"));
        CHECK(GetWindowLongPtrW(otherProcess, GWL_EXSTYLE) & WS_EX_TOOLWINDOW);
        CHECK(TaggedDesktopMatches(otherProcess, deskA));
        checkSealed(otherDesktop, deskB);
        checkSealed(otherDesktopSibling, deskB);

        CHECK(TryUnHideWindowsForAltTab(secondDesktop));
        CHECK(g_snapUnhiddenWindows.size() == 2);
        for (HWND h : {otherDesktop.h, otherDesktopSibling.h}) {
            CHECK(std::find(g_snapUnhiddenWindows.begin(), g_snapUnhiddenWindows.end(), h) !=
                g_snapUnhiddenWindows.end());
            CHECK(IsWindowVisible(h) && !(GetWindowLongPtrW(h, GWL_EXSTYLE) & WS_EX_TOOLWINDOW));
        }
        FinishAltTabPreview(otherDesktop, firstDesktop); // Desktop changed while Alt was held.
        checkSealed(otherDesktop, deskB);
        checkSealed(otherDesktopSibling, deskB);

        CHECK(TryUnHideWindowsForAltTab(secondDesktop));
        ExecuteCollectRestore(); // The coordinator invokes this on Escape.
        CHECK(!g_InSnapWindows && g_snapUnhiddenWindows.empty());
        checkSealed(otherDesktop, deskB);
        checkSealed(otherDesktopSibling, deskB);

        CHECK(TryUnHideWindowsForAltTab(secondDesktop));
        FinishAltTabPreview(otherDesktop, secondDesktop);
        CHECK(!g_InSnapWindows && g_snapUnhiddenWindows.empty());
        for (HWND h : {otherDesktop.h, otherDesktopSibling.h}) {
            CHECK(IsWindowVisible(h) && (GetWindowLongPtrW(h, GWL_EXSTYLE) & WS_EX_APPWINDOW));
            CHECK(!GetPropW(h, L"MTU_EXE_TAG") && !GetPropW(h, L"MTU_VD_TAG"));
            CHECK(!g_sealedWindowStates.contains(h));
        }
        CHECK(TaggedDesktopMatches(otherProcess, deskA));
        RestoreAllSealedWindows();
        return;
    }
    if (name == "uwp_win_c_close") {
        WNDCLASSW wc{};
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"ApplicationFrameWindow";
        CHECK(RegisterClassW(&wc));
        Window frame(WS_EX_APPWINDOW, nullptr, wc.lpszClassName);
        ShowWindow(frame, SW_SHOWNOACTIVATE);
        HideCurrentFilter filter{GetRealProcessName(frame), +[](HWND, GUID& id, bool) {
            id = GUID{2};
            return true;
        }};
        CHECK(!filter.process.empty() && filter.process != L"Unknown");
        CHECK(EnumHideWindowsProc(frame, reinterpret_cast<LPARAM>(&filter)));
        CHECK(IsWindowVisible(frame) && !g_sealedWindowStates.contains(frame.h));
        CHECK(!GetPropW(frame, L"MTU_EXE_TAG") && !GetPropW(frame, L"MTU_VD_TAG"));
        MSG message{};
        CHECK(PeekMessageW(&message, frame, WM_CLOSE, WM_CLOSE, PM_REMOVE));
        DispatchMessageW(&message);
        CHECK(!IsWindow(frame));
        return;
    }
    if (name == "alt_tab_chord") {
        INPUT input[6]{};
        CHECK(BuildAltTabInput(input, false, false, true) == 6);
        constexpr WORD keys[] = {VK_MENU, VK_SHIFT, VK_TAB, VK_TAB, VK_SHIFT, VK_MENU};
        for (size_t i = 0; i < std::size(keys); ++i) {
            CHECK(input[i].type == INPUT_KEYBOARD && input[i].ki.wVk == keys[i]);
            CHECK(input[i].ki.dwFlags == (i >= 3 ? KEYEVENTF_KEYUP : 0));
        }
        CHECK(BuildAltTabInput(input, true, false, false) == 2);
        CHECK(input[0].ki.wVk == VK_TAB && input[0].ki.dwFlags == 0);
        CHECK(input[1].ki.wVk == VK_TAB && input[1].ki.dwFlags == KEYEVENTF_KEYUP);
        return;
    }
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
    } else if (name == "desktop_guid_atom") {
        GUID expected{};
        CHECK(SUCCEEDED(CLSIDFromString(
            L"{4fd230e8-260c-4fa9-9234-151155a7b739}", &expected)));
        ATOM atom = GlobalAddAtomW(L"{4fd230e8-260c-4fa9-9234-151155a7b739}");
        CHECK(atom != 0);
        CHECK(SetPropW(w, L"MTU_VD_TAG", reinterpret_cast<HANDLE>((UINT_PTR)atom)));
        GUID actual{};
        CHECK(ReadTaggedDesktopGuid(w, actual));
        CHECK(IsEqualGUID(expected, actual));
        CHECK(TaggedDesktopMatches(w, expected));
        ClearSealMarkers(w);
        CHECK(!GetPropW(w, L"MTU_VD_TAG"));
    } else if (name == "legacy_desktop_name_rejected") {
        ATOM atom = GlobalAddAtomW(L"My Custom Desktop");
        CHECK(atom != 0);
        CHECK(SetPropW(w, L"MTU_VD_TAG", reinterpret_cast<HANDLE>((UINT_PTR)atom)));
        GUID actual{};
        CHECK(!ReadTaggedDesktopGuid(w, actual));
        CHECK(!CanUnsealOnCurrentDesktop(w));
        ClearSealMarkers(w);
    } else if (name == "wrong_desktop_guid_rejected") {
        // Even if the system desktop manager is unavailable in CI,
        // an invalid desktop identity must fail closed.
        ATOM atom = GlobalAddAtomW(L"{00000000-0000-0000-0000-000000000001}");
        CHECK(atom != 0);
        CHECK(SetPropW(w, L"MTU_VD_TAG", reinterpret_cast<HANDLE>((UINT_PTR)atom)));
        GUID unexpected{};
        CHECK(SUCCEEDED(CLSIDFromString(
            L"{00000000-0000-0000-0000-000000000002}", &unexpected)));
        CHECK(!TaggedDesktopMatches(w, unexpected));
        CHECK(!CanUnsealOnCurrentDesktop(w));
        ClearSealMarkers(w);
    } else if (name == "minimized_stays_minimized") {
        ShowWindow(w, SW_SHOWNOACTIVATE);
        ShowWindow(w, SW_MINIMIZE);
        CHECK(IsIconic(w));
        const LONG_PTR original = GetWindowLongPtrW(w, GWL_EXSTYLE);
        HideWindowAndHideTaskbarIcon(w);
        CHECK(IsIconic(w));
        CHECK(!IsOriginalWindowEligibleForPreview(w));
        ShowWindowAndShowTaskbarIcon(w);
        CHECK(IsIconic(w));
        CHECK(GetWindowLongPtrW(w, GWL_EXSTYLE) == original);
    } else if (name == "hidden_stays_hidden") {
        CHECK(!IsWindowVisible(w));
        const LONG_PTR original = GetWindowLongPtrW(w, GWL_EXSTYLE);
        HideWindowAndHideTaskbarIcon(w);
        CHECK(!IsWindowVisible(w));
        CHECK(!IsOriginalWindowEligibleForPreview(w));
        ShowWindowAndShowTaskbarIcon(w);
        CHECK(!IsWindowVisible(w));
        CHECK(GetWindowLongPtrW(w, GWL_EXSTYLE) == original);
    } else if (name == "seal_idempotent") {
        ShowWindow(w, SW_SHOWNOACTIVATE);
        const LONG_PTR original = GetWindowLongPtrW(w, GWL_EXSTYLE);
        HideWindowAndHideTaskbarIcon(w);
        CHECK(g_sealedWindowStates.contains(w.h));
        const size_t count = g_sealedWindowStates.size();
        HideWindowAndHideTaskbarIcon(w);
        CHECK(g_sealedWindowStates.size() == count);
        ShowWindowAndShowTaskbarIcon(w);
        CHECK(GetWindowLongPtrW(w, GWL_EXSTYLE) == original);
        CHECK(!g_sealedWindowStates.contains(w.h));
    } else if (name == "restore_tool_style") {
        ShowWindow(w, SW_SHOWNOACTIVATE);
        const LONG_PTR original = GetWindowLongPtrW(w, GWL_EXSTYLE);
        SetWindowLongPtrW(w, GWL_EXSTYLE, (original & ~WS_EX_APPWINDOW) | WS_EX_TOOLWINDOW);
        const LONG_PTR tool = GetWindowLongPtrW(w, GWL_EXSTYLE);
        HideWindowAndHideTaskbarIcon(w);
        CHECK(!IsOriginalWindowEligibleForPreview(w));
        CHECK(!ShowWindowAndShowTaskbarIcon(w, true));
        CHECK(ShowWindowAndShowTaskbarIcon(w));
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
        CHECK(SetPropW(w, L"MTU_EXE_TAG", reinterpret_cast<HANDLE>(1)));
        g_InSnapWindows = true;
        SuppressHiddenWindows(w, EVENT_OBJECT_SHOW);
        CHECK(!(GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW));
        g_InSnapWindows = false;
        SuppressHiddenWindows(w, EVENT_OBJECT_FOCUS);
        CHECK(!(GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW));
        // Process tag alone must NOT override virtual desktop isolation.
        SuppressHiddenWindows(w, EVENT_OBJECT_SHOW);
        CHECK(!(GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW));
        RemovePropW(w, L"MTU_EXE_TAG");
        SuppressHiddenWindows(w, EVENT_OBJECT_SHOW);
        CHECK(!(GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW));
    } else if (name == "collect") {
        ShowWindow(w, SW_SHOWNOACTIVATE);
        ATOM process = GlobalAddAtomW(L"preview-fixture.exe");
        CHECK(process);
        CHECK(SetPropW(w, L"MTU_EXE_TAG", reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(process))));
        HideWindowAndHideTaskbarIcon(w);
        CHECK(ShowWindowAndShowTaskbarIcon(w, true));
        CHECK(IsWindowVisible(w));
        CHECK(!(GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW));

        g_InSnapWindows = true;
        g_snapUnhiddenWindows = {w, nullptr};
        CHECK(TryUnHideWindowsForAltTab());
        CHECK(g_snapUnhiddenWindows.size() == 2);
        ExecuteCollectRestore();
        CHECK(g_snapUnhiddenWindows.empty());
        CHECK(!g_InSnapWindows);
        CHECK(GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW);
        CHECK(ReadTaggedExe(w) == L"preview-fixture.exe");
        ExecuteCollectRestore();
        CHECK(g_snapUnhiddenWindows.empty());
        CHECK(ShowWindowAndShowTaskbarIcon(w));
        CHECK(ReadTaggedExe(w).empty());
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
        CHECK(SetPropW(w, L"MTU_EXE_TAG", reinterpret_cast<HANDLE>(1)));
        CHECK(EnumWindowToggleProc(w, reinterpret_cast<LPARAM>(&context)));
        CHECK(found.empty());
        RemovePropW(w, L"MTU_EXE_TAG");
        CHECK(EnumWindowToggleProc(w, reinterpret_cast<LPARAM>(&context)));
        // Headless CI can lack the Virtual Desktop COM service. The filter
        // fails closed in that case instead of collecting a foreign window.
        if (IsWindowOnCurrentVirtualDesktop(w))
            CHECK(found.size() == 1 && found.front() == w.h);
        else
            CHECK(found.empty());
    } else throw std::runtime_error("unknown case");
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try { run(argv[1]); std::cout << "PASS: " << argv[1] << '\n'; return 0; }
    catch (const std::exception& e) { std::cerr << "FAIL: " << argv[1] << ": " << e.what() << '\n'; return 1; }
}
