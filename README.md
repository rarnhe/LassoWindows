# LassoWindows

LassoWindows 是基于 Win32、COM 与 C++/WinRT 的后台窗口管理工具。程序通过键盘、鼠标及窗口事件钩子，使用 EXE 文件名（`MTU_EXE_TAG`）和虚拟桌面 GUID（`MTU_VD_TAG`）两个维度管理窗口。以下说明基于 `LassoWindows.cpp` 当前实现。

## 功能与操作

| 操作 | 当前行为 |
| --- | --- |
| Win + C | 封印各虚拟桌面上进程名相同的可见顶级窗口，并记录各窗口自己的桌面 GUID。Explorer 仅处理文件夹窗口。对匹配的 `ApplicationFrameWindow` 外框发送 `WM_CLOSE`。 |
| Shift + Win + C | 在所有虚拟桌面隐藏其他应用窗口，保留当前前台应用；普通窗口按各自桌面 GUID 封印，其他 UWP 外框发送 `WM_CLOSE`。 |
| Alt + Tab | 先读取当前桌面的 GUID，再临时恢复带有效 `MTU_EXE_TAG` 且 `MTU_VD_TAG` 匹配该 GUID 的封印窗口。Alt+Tab 缩略卡片的窗口图标和临时任务栏按钮均使用按 DWM `AccentColor` 染色的 Webdings 蛛网图标。松开 Alt 后，选中窗口恢复原图标，其余窗口重新封印。Esc 取消本次预览。 |
| 点击主屏任务栏最右侧 10 像素 | 将筛选后的主屏当前桌面普通窗口透明化并启用鼠标穿透；仅临时增加 `WS_EX_LAYERED` / `WS_EX_TRANSPARENT`，不增加 `WS_EX_TOOLWINDOW`；再次点击右下角或点击任务栏应用图标时，全部恢复原始样式与透明度。 |
| 再次打开同应用窗口或通过关联文件唤醒 | 事件处理尝试恢复同进程、同桌面的已隐藏窗口并清理标签。 |

程序在封印前保存窗口原始扩展样式、可读取的分层窗口透明度、可见性及最小化状态；无法安全读取原有分层属性的窗口会跳过封印。封印时通过透明度为 0、鼠标穿透、工具窗口样式及任务栏接口隐藏窗口，并使用 `MTU_EXE_TAG` / `MTU_VD_TAG` Atom 属性记录进程名和官方虚拟桌面 GUID。恢复时撤销相关标签，并按照快照恢复样式与 Alpha；Alt+Tab 临时预览会恢复透明度，临时加入 `WS_EX_APPWINDOW` 并显式显示窗口，同时用带蛛网的大/小图标更新缩略卡片，并为同一窗口设置临时 AppUserModelID 和同一蛛网图标的 `.ico` 资源供 Explorer 创建预览按钮。选中窗口或取消预览时，恢复原始图标和任务栏分组，清理临时图标文件。最终解封时恢复原始扩展样式并清除快照。原本是工具窗口的窗口不进入预览。`Win+C` 跨虚拟桌面封印同名 EXE 窗口；`Shift+Win+C` 以当前前台应用为例外，跨所有虚拟桌面封印其他应用，UWP 外框仅发送常规 `WM_CLOSE` 请求。不使用未公开的 Shell COM 接口，也不设置或修改 DWM Cloak；Alt+Tab 仅保留对系统 `DWMWA_CLOAKED` 状态的只读检查。应用接受关闭后可通过开始菜单或快捷方式重新打开。Alt+Tab 临时恢复当前桌面的封印窗口，使用独立于 Win+Tab 任务视图的状态。前台没有可用于识别桌面的普通窗口时，先检查当前桌面的封印窗口，再优先读取当前交互会话的 Explorer 虚拟桌面 GUID，最后回退到未按会话区分的注册表值。如果收网时已切换虚拟桌面，本轮预览全部重新封印。右下角触发区位于主任务栏矩形最右侧，宽 10 个物理像素。第一次点击后，主屏当前桌面的目标窗口临时变为 Alpha 0 并启用鼠标穿透，窗口保留原有任务栏及任务视图样式。单击选中主屏桌面图标或任意次数右键单击该图标时继续保持透明；仅左键双击打开图标时恢复。再次点击触发区、点击任务栏应用图标、点击桌面空白处或其他区域、拖动、滚轮和键盘操作均恢复原样。任务栏应用图标仍由 Explorer 执行原本的点击动作。单纯移动鼠标保持当前状态。临时透明状态不纳入程序退出清理路径。

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

测试位于独立的 `test/` 目录，通过 CTest 驱动 43 个用例，直接编译并调用 `LassoWindows.cpp` 的实际实现，不执行 `WinMain`，也不安装全局钩子。测试创建并销毁自己的 Win32 窗口；INI 临时文件只放在测试可执行文件旁，用例串行运行并自动清理。

```powershell
ctest --test-dir out/test -C Release --output-on-failure --no-tests=error --output-junit test-results.xml
./test/write_report.ps1
```

覆盖 INI 正常/缺失/无效索引/含逗号路径、进程识别、窗口过滤、坐标与可见性、隐藏/恢复样式与透明度、自定义窗口样式/分层 Alpha/临时预览快照/回调上下文、压制条件、跨桌面隐藏其他应用与 UWP WM_CLOSE、主任务栏右侧 10 像素边界、透明化/穿透/样式恢复、桌面图标例外和任务栏点击恢复全部窗口、收网状态、图标缓存、预览蛛网图标生成/幂等/选中恢复/取消后重新封印、任务栏图标文件加载和分组属性恢复，以及主屏筛选过滤；新增 GUID Atom 解析、EXE 文件名与桌面 GUID 双标签读写、旧名称标签拒绝、错误 GUID 拒绝，以及最小化/原本隐藏的窗口不被强行显示。跨桌面用例从 `Win+C` 的处理入口验证同名 EXE 窗口的双标签；新增桌面切换回归用例模拟 Shell 前台、封印窗口无法通过 VDM 判定当前桌面、会话级注册表正确值与全局过期值并存，确认 Alt+Tab 只预览当前桌面的窗口。用例创建 Win32 窗口并模拟桌面归属。

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
