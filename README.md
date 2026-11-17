# LassoWindows

LassoWindows 是基于 Win32、COM 与 C++/WinRT 的后台窗口管理工具。程序通过键盘、鼠标及窗口事件钩子，使用 EXE 文件名（`MTU_EXE_TAG`）和虚拟桌面 GUID（`MTU_VD_TAG`）两个维度管理窗口。以下说明基于 `LassoWindows.cpp` 当前实现。

## 功能与操作

| 操作 | 当前行为 |
| --- | --- |
| Win + C | 封印各虚拟桌面上进程名相同的可见顶级窗口，并记录各窗口自己的桌面 GUID。Explorer 仅处理文件夹窗口。对匹配的 `ApplicationFrameWindow` 外框发送 `WM_CLOSE`。 |
| Shift + Win + C | 在所有虚拟桌面隐藏其他应用窗口，保留当前前台应用；普通窗口按各自桌面 GUID 封印，其他 UWP 外框发送 `WM_CLOSE`。 |
| Alt + Tab | 按当前桌面 GUID 和 `MTU_EXE_TAG` / `MTU_VD_TAG` 筛选封印窗口，在 Windows 原生切换器中按 `MTU_EXE_TAG` 分组显示 Lasso 代理卡片。代理卡片左对齐显示 `EXE 名称 「窗口数量」`，下方按 Z 序逐行列出真实窗口标题。Alt+Tab 标题图标保留黑色蛛网遮罩；代理窗口的任务栏图标使用主题强调色蛛网遮罩。缩略图本身不再叠加蛛网图案。选中代理并松开 Alt 时还原真实窗口及本桌面同进程窗口，并将焦点交给选中的目标。Esc 清理代理。|
| 点击主屏任务栏的“显示桌面”按钮 | 将当前虚拟桌面主屏上可见、未最小化的普通顶层窗口变为透明和鼠标穿透，任务栏图标保留，并按原枚举顺序记录窗口状态。再次点击，或有新的应用顶层窗口获得前台、创建或显示时，按记录顺序恢复。 |
| 再次打开同应用窗口或通过关联文件唤醒 | 事件处理尝试恢复同进程、同桌面的已隐藏窗口并清理标签。 |

Win+C 封印前保存目标窗口的扩展样式、可读取的透明度和显示状态，用每扇窗口自己的虚拟桌面 GUID 写入 `MTU_VD_TAG`。Alt+Tab 优先从当前桌面的普通窗口解析 GUID，当前台是 Shell 时可用会话范围的 Explorer 注册表值。候选窗口按双标签筛选，封印后的 Shell Cloak 状态不会挡住代理卡片的创建。

Alt+Tab 期间，真实目标继续封印。当前虚拟桌面上同一 `MTU_EXE_TAG` 的窗口共用一张 Lasso 代理卡片，Lasso 在预览图内左对齐绘制 EXE 名称和用 `「」` 包裹的窗口数量，下方按 Z 序从上到下逐行左对齐绘制真实窗口标题。DWM 缩略图使用透明背景和主题强调色文字，字体为微软雅黑 19 px，宽度按最宽真实窗口标题计算，高度从 137 px 起按行数增长。Windows Shell 负责原生切换器的外层标签排版。代理窗口的 Alt+Tab 标题图标继续使用黑色蛛网遮罩，任务栏图标则使用主题强调色蛛网遮罩；原始应用图标与临时图标资源仍按预览生命周期保存和恢复。Explorer 决定卡片的最终排序。松开 Alt 后，确认当前桌面仍匹配才还原所选组。取消预览会销毁代理并清理任务栏临时图标资源。直接启动应用或通过文件关联打开文件时，创建或激活窗口也可解除当前虚拟桌面上该应用的封印。

主任务栏的“显示桌面”按钮通过原生控件或 Windows UI Automation 实际命中。确认点击后由 Lasso 批量最小化当前虚拟桌面上的主屏窗口，并记录每个 HWND。再次点击或激活应用时解除这批窗口的最小化，原本已最小化的窗口保持原状。若当前 Explorer 无法识别该控件，点击继续交给系统。Shell、切换器、Lasso 自有窗口、子窗口和批量最小化引发的排队事件会被过滤。已销毁的句柄会剔除。

任务栏相关代码会备份/恢复 AppUserModelID，缓存原始图标，并尝试设置窗口快照与叠加图标。启动时以命名互斥量限制单实例，在独立协调线程初始化 WinRT/COM、创建消息窗口并注册窗口事件 Hook；低级键盘和鼠标 Hook 只识别输入、投递消息，窗口枚举/COM/DWM 等耗时操作由协调线程串行处理。正常退出会尝试恢复被封印窗口并解除钩子。程序目前没有设置界面或托盘退出入口，必要时可在任务管理器结束进程。

## 环境与构建

需要 Windows、Visual Studio 2022 或更新版本的 MSVC C++ 工具，以及包含 C++/WinRT 头文件的 Windows SDK。项目使用 C++20；不支持 Linux 或 macOS 构建。Windows 11 是主要交互验证环境，其他 Windows 版本需自行验证 Shell 行为。

```powershell
cmake -S . -B out/test -A x64 -DBUILD_TESTING=ON
cmake --build out/test --config Release --parallel
```

运行 `out/test/Release/LassoWindows.exe`。如需快捷方式图标，可在应用可执行文件同目录放置 ANSI（系统代码页）编码的 `desktop.ini`：

```ini
[.ShellClassInfo]
IconResource=C:\Windows\System32\shell32.dll,3
```

解析使用最后一个逗号分隔路径与索引，支持负索引；索引无法转换时回退为 0。快捷方式已经存在时跳过创建，因此修改 INI 不会自动更新已有快捷方式。

## 自动测试与双轨报告

测试位于独立的 `test/` 目录，通过 CTest 驱动 45 个用例，直接编译并调用 `LassoWindows.cpp` 的实际实现，不执行 `WinMain`，也不安装全局钩子。测试创建并销毁自己的 Win32 窗口；INI 临时文件只放在测试可执行文件旁，用例串行运行并自动清理。

```powershell
ctest --test-dir out/test -C Release --output-on-failure --no-tests=error --output-junit test-results.xml
./test/write_report.ps1
```

用例覆盖配置解析、窗口筛选、封印与恢复、UWP 关闭请求、主任务栏“显示桌面”控件命中、任务栏样式保留与批量恢复、双标签桌面隔离、重复 Alt+Tab、按 EXE 分组的代理映射、MTU_EXE_TAG 标题、Z 序标题列表及其左对齐、按最宽标题和行数排版的透明缩略图，以及黑色 Alt+Tab 图标、强调色任务栏图标和资源恢复。跨桌面用例模拟 Windows 桌面归属与 Explorer 的会话级桌面 GUID，验证切换后只为当前虚拟桌面的封印窗口建立代理。

`.github/workflows/test_cpp.yml` 在 `windows-latest` 上构建应用与测试，响应 main 推送、指向 main 的 PR 和手动触发。参考 [xlsx2md](https://github.com/rarnhe/xlsx2md) 的报告方式：

- 每次运行将结果写入 Actions Summary，并上传 `test_report.md`、JUnit 和诊断日志为 artifact；失败阶段的退出码和末尾日志也会写入报告。
- main 推送运行会自动提交根目录 `test_report.md`，仅报告变化不会再次触发流水线。仓库需允许 Actions 写入内容；分支保护若禁止机器人直接推送，报告提交步骤会失败，artifact 仍保留。
- PR（含 fork）和手动运行通过 Summary/artifact 保存本次 Markdown 报告，不回写分支。仓库中的报告包含生成时的提交与运行来源，请据此判断是否对应当前代码。

自动测试不等于完整桌面端到端验证。全局快捷键、Alt+Tab 缩略图、UWP、任务栏视觉效果、双屏/混合 DPI、命名虚拟桌面切换与通知需在交互式桌面人工检查。

## 当前实现限制

- Win+Tab 的窗口转移仍为 TODO：当前只检测中文标题“任务视图”并显示提示，没有实现自动转移窗口。
- `MTU_VD_TAG` 现在保存官方窗口桌面 GUID，使用 `IVirtualDesktopManager` 校验当前桌面。旧版桌面**名称**标签不会被解封（fail closed），以防止同名或改名导致错误唤醒；切换版本前建议主动解封旧版封印窗口。
- 已增加原始扩展样式、可读取透明度等快照及正常退出恢复；但 Windows 的特殊分层窗口效果、受权限保护的窗口和强制结束进程后的恢复仍无法保证，必须实机验证。
- 如果只识别到共享宿主，`Win+C` 只请求关闭前台 UWP 外框。应用可能显示保存提示或拒绝关闭。固定的任务栏启动图标由 Explorer 管理。实际按钮和 Alt+Tab 缩略图需要在交互式桌面验证。
