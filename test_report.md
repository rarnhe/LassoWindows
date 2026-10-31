# C++ Windows 测试报告

- 状态：通过 (Passed)
- 时间（UTC）：2026-10-31 09:42:32
- 测试源码基准提交：49bd5b9f09b358dd152a434d4a5fd37a5d1a6d5d
- 执行记录：https://github.com/rarnhe/LassoWindows/actions/runs/41189725218
- 平台：Windows / MSVC / C++20 / Win32 + Windows SDK
- 阶段：Configure=success / Build=success / CTest=success
- 统计：通过 29 / 失败 0 / 跳过 0 / 共 29

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

## 失败详情
无用例失败详情；若整体失败，请检查 Actions 阶段和构建日志。

## 覆盖范围
真实编译 LassoWindows.cpp，验证 INI 图标解析、进程识别、窗口过滤/坐标/可见性、隐藏恢复样式、压制条件、收网状态、图标缓存和主屏收集过滤。测试仅操作自己创建的窗口，不启动 WinMain 或安装全局钩子。

全局快捷键、Alt+Tab 实际缩略图、Explorer/UWP、任务栏视觉效果、多显示器/不同 DPI、虚拟桌面切换与通知仍需交互式人工验证；这些项目未计入自动测试通过数。
