# C++ Windows 测试报告

- 状态：通过 (Passed)
- 时间（UTC）：2026-11-15 18:32:10
- 测试源码基准提交：5285cbb1a364faa7796722fcea073dfb67e25cbc
- 执行记录：https://github.com/rarnhe/LassoWindows/actions/runs/43747245995
- 平台：Windows / MSVC / C++20 / Win32 + Windows SDK
- 阶段：Configure=success / Build=success / CTest=success
- 统计：通过 36 / 失败 0 / 跳过 0 / 共 36

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
| uwp_frame_close | 通过 |
| uwp_frame_recovery | 通过 |
| uwp_shell_switcher_state | 通过 |
| alt_tab_chord | 通过 |
| alt_tab_consecutive_previews | 通过 |
| cross_desktop_win_c | 通过 |
| uwp_win_c_close | 通过 |

## 失败详情
无用例失败详情；若整体失败，请检查 Actions 阶段和构建日志。

## 覆盖范围
真实编译 LassoWindows.cpp，验证 INI 图标解析、进程识别、窗口过滤/坐标/可见性、隐藏恢复样式、压制条件、收网状态、跨虚拟桌面同进程封印及逐窗 GUID 标签、Alt+Tab 当前桌面 GUID 筛选、同进程解封、连续预览、临时应用窗口样式和最终原样恢复、收网计时器隔离、Win+C 向模拟 UWP 外框派发 WM_CLOSE、图标缓存和主屏收集过滤。测试仅操作自己创建的窗口，不启动 WinMain 或安装全局钩子。

全局快捷键、Alt+Tab 实际缩略图、Explorer/UWP、任务栏视觉效果、多显示器/不同 DPI、虚拟桌面切换与通知仍需交互式人工验证；这些项目未计入自动测试通过数。
