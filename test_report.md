# C++ Windows 测试报告

- 状态：通过 (Passed)
- 时间（UTC）：2026-11-16 16:51:29
- 测试源码基准提交：8a4b60dd119a316227a6b193659e545d712033ba
- 执行记录：https://github.com/rarnhe/LassoWindows/actions/runs/43911960772
- 平台：Windows / MSVC / C++20 / Win32 + Windows SDK
- 阶段：Configure=success / Build=success / CTest=success
- 统计：通过 44 / 失败 0 / 跳过 0 / 共 44

## GitHub Actions 阶段状态

| 阶段 | 结果 |
| --- | --- |
| Configure | success |
| Build | success |
| CTest | success |

## 测试用例结果

| 测试 | 结果 |
| --- | --- |
| ini_missing | 通过 |
| ini_no_key | 通过 |
| ini_index | 通过 |
| ini_negative | 通过 |
| ini_no_index | 通过 |
| ini_invalid | 通过 |
| ini_empty | 通过 |
| ini_comma | 通过 |
| process_names | 通过 |
| invalid_process | 通过 |
| window_filter | 通过 |
| rectangle | 通过 |
| visibility | 通过 |
| hide_restore | 通过 |
| desktop_guid_atom | 通过 |
| legacy_desktop_name_rejected | 通过 |
| wrong_desktop_guid_rejected | 通过 |
| minimized_stays_minimized | 通过 |
| hidden_stays_hidden | 通过 |
| seal_idempotent | 通过 |
| restore_tool_style | 通过 |
| restore_layered_alpha | 通过 |
| preview_preserves_snapshot | 通过 |
| hide_others_context | 通过 |
| untracked_restore | 通过 |
| suppress | 通过 |
| collect | 通过 |
| icon_cache | 通过 |
| toggle_filter | 通过 |
| hide_others_all_desktops | 通过 |
| primary_hide_roundtrip | 通过 |
| primary_hide_events | 通过 |
| show_desktop_button_hit_test | 通过 |
| uwp_frame_close | 通过 |
| exe_tag_atom | 通过 |
| alt_tab_after_desktop_switch | 通过 |
| alt_tab_chord | 通过 |
| alt_tab_consecutive_previews | 通过 |
| cross_desktop_win_c | 通过 |
| uwp_win_c_close | 通过 |
| sealed_preview_icon_roundtrip | 通过 |
| preview_icon_selection_restore | 通过 |
| preview_icon_reseal_restore | 通过 |
| preview_taskbar_icon_roundtrip | 通过 |

## 失败详情

## 覆盖范围
真实编译 LassoWindows.cpp，验证 INI 图标解析、EXE 文件名与虚拟桌面 GUID 双标签、跨虚拟桌面同名 EXE 封印、Shift+Win+C 跨全部虚拟桌面隐藏其他应用及 UWP WM_CLOSE、会话级虚拟桌面 GUID 优先解析、切换桌面后 Alt+Tab 仅预览当前桌面窗口、主屏原生 Show Desktop 按钮精确命中、主屏窗口透明化及还原、桌面图标点击例外/双击/右键规则、临时透明样式不添加 WS_EX_TOOLWINDOW、键盘与其他鼠标操作恢复、关闭/清理边界、图标缓存和主屏筛选条件。测试仅操作自己创建的窗口，不启动 WinMain 或安装全局钩子。

全局快捷键、Alt+Tab 实际缩略图、Explorer/UWP、任务栏视觉效果、多显示器/不同 DPI、虚拟桌面切换与通知仍需交互式人工验证；这些项目未计入自动测试通过数。
