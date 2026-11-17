# C++ Windows 测试报告

- 状态：失败或未完整执行 (Failed)
- 时间（UTC）：2026-11-17 14:11:14
- 测试源码基准提交：15fe635c37809c483cb31e364386de66a6b2d64d
- 执行记录：https://github.com/rarnhe/LassoWindows/actions/runs/44100174364
- 平台：Windows / MSVC / C++20 / Win32 + Windows SDK
- 阶段：Configure=success / Build=success / CTest=failure
- 统计：通过 44 / 失败 1 / 跳过 0 / 共 45

## GitHub Actions 阶段状态

| 阶段 | 结果 |
| --- | --- |
| Configure | success |
| Build | success |
| CTest | failure |

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
| proxy_title_thumbnail | 失败 |
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

### proxy_title_thumbnail
<pre>
FAIL: proxy_title_thumbnail: layout.height == 60

</pre>

### CTest 阶段输出
GitHub Actions 错误：Error: Process completed with exit code 8.
<pre>11/45 Test #11: window_filter ....................   Passed    0.02 sec
      Start 12: rectangle
12/45 Test #12: rectangle ........................   Passed    0.02 sec
      Start 13: visibility
13/45 Test #13: visibility .......................   Passed    0.02 sec
      Start 14: hide_restore
14/45 Test #14: hide_restore .....................   Passed    0.03 sec
      Start 15: desktop_guid_atom
15/45 Test #15: desktop_guid_atom ................   Passed    0.02 sec
      Start 16: legacy_desktop_name_rejected
16/45 Test #16: legacy_desktop_name_rejected .....   Passed    0.02 sec
      Start 17: wrong_desktop_guid_rejected
17/45 Test #17: wrong_desktop_guid_rejected ......   Passed    0.02 sec
      Start 18: minimized_stays_minimized
18/45 Test #18: minimized_stays_minimized ........   Passed    0.03 sec
      Start 19: hidden_stays_hidden
19/45 Test #19: hidden_stays_hidden ..............   Passed    0.04 sec
      Start 20: seal_idempotent
20/45 Test #20: seal_idempotent ..................   Passed    0.04 sec
      Start 21: restore_tool_style
21/45 Test #21: restore_tool_style ...............   Passed    0.02 sec
      Start 22: restore_layered_alpha
22/45 Test #22: restore_layered_alpha ............   Passed    0.03 sec
      Start 23: preview_preserves_snapshot
23/45 Test #23: preview_preserves_snapshot .......   Passed    0.07 sec
      Start 24: hide_others_context
24/45 Test #24: hide_others_context ..............   Passed    0.04 sec
      Start 25: untracked_restore
25/45 Test #25: untracked_restore ................   Passed    0.03 sec
      Start 26: suppress
26/45 Test #26: suppress .........................   Passed    0.02 sec
      Start 27: collect
27/45 Test #27: collect ..........................   Passed    0.12 sec
      Start 28: proxy_title_thumbnail
28/45 Test #28: proxy_title_thumbnail ............***Failed    0.02 sec
FAIL: proxy_title_thumbnail: layout.height == 60

      Start 29: icon_cache
29/45 Test #29: icon_cache .......................   Passed    0.02 sec
      Start 30: toggle_filter
30/45 Test #30: toggle_filter ....................   Passed    0.03 sec
      Start 31: hide_others_all_desktops
31/45 Test #31: hide_others_all_desktops .........   Passed    0.07 sec
      Start 32: primary_hide_roundtrip
32/45 Test #32: primary_hide_roundtrip ...........   Passed    0.05 sec
      Start 33: primary_hide_events
33/45 Test #33: primary_hide_events ..............   Passed    0.11 sec
      Start 34: show_desktop_button_hit_test
34/45 Test #34: show_desktop_button_hit_test .....   Passed    0.03 sec
      Start 35: uwp_frame_close
35/45 Test #35: uwp_frame_close ..................   Passed    0.03 sec
      Start 36: exe_tag_atom
36/45 Test #36: exe_tag_atom .....................   Passed    0.02 sec
      Start 37: alt_tab_after_desktop_switch
37/45 Test #37: alt_tab_after_desktop_switch .....   Passed    0.11 sec
      Start 38: alt_tab_chord
38/45 Test #38: alt_tab_chord ....................   Passed    0.02 sec
      Start 39: alt_tab_consecutive_previews
39/45 Test #39: alt_tab_consecutive_previews .....   Passed    0.16 sec
      Start 40: cross_desktop_win_c
40/45 Test #40: cross_desktop_win_c ..............   Passed    0.25 sec
      Start 41: uwp_win_c_close
41/45 Test #41: uwp_win_c_close ..................   Passed    0.04 sec
      Start 42: sealed_preview_icon_roundtrip
42/45 Test #42: sealed_preview_icon_roundtrip ....   Passed    0.04 sec
      Start 43: preview_icon_selection_restore
43/45 Test #43: preview_icon_selection_restore ...   Passed    0.05 sec
      Start 44: preview_icon_reseal_restore
44/45 Test #44: preview_icon_reseal_restore ......   Passed    0.06 sec
      Start 45: preview_taskbar_icon_roundtrip
45/45 Test #45: preview_taskbar_icon_roundtrip ...   Passed    0.06 sec

98% tests passed, 1 tests failed out of 45

Total Test time (real) =   2.04 sec

The following tests FAILED:
	 28 - proxy_title_thumbnail (Failed)
Errors while running CTest
Exit code: 8</pre>

## 覆盖范围
真实编译 LassoWindows.cpp，验证 INI 图标解析、EXE 文件名与虚拟桌面 GUID 双标签、跨虚拟桌面同名 EXE 封印、Shift+Win+C 跨全部虚拟桌面隐藏其他应用及 UWP WM_CLOSE、会话级虚拟桌面 GUID 优先解析、Alt+Tab 按 MTU_EXE_TAG 合并代理并以 EXE 名称作为标题、代理缩略图 60 px 高/微软雅黑 19 px/按标题实测宽度/透明背景且不叠加蛛网、Alt+Tab 标题图标黑色蛛网与任务栏强调色蛛网图标及图标资源恢复、主屏原生 Show Desktop 按钮精确命中、主屏窗口透明化及还原、桌面图标点击例外/双击/右键规则、临时透明样式不添加 WS_EX_TOOLWINDOW、键盘与其他鼠标操作恢复、关闭/清理边界、图标缓存和主屏筛选条件。测试仅操作自己创建的窗口，不启动 WinMain 或安装全局钩子。

全局快捷键、Alt+Tab 实际缩略图、Explorer/UWP、任务栏视觉效果、多显示器/不同 DPI、虚拟桌面切换与通知仍需交互式人工验证；这些项目未计入自动测试通过数。
