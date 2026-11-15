# LassoWindows

LassoWindows 是基于 Win32、COM 与 C++/WinRT 的后台窗口管理工具。程序通过键盘、鼠标及窗口事件钩子，按应用进程名管理封印窗口。以下说明基于 `LassoWindows.cpp` 当前实现。

## 功能与操作

| 操作 | 当前行为 |
| --- | --- |
| Win + C | 封印进程名相同的可见顶级窗口，并写入 `MTU_PROC_TAG`。Explorer 仅处理文件夹窗口。对匹配的 `ApplicationFrameWindow` 外框发送 `WM_CLOSE`。 |
| Shift + Win + C | 尝试隐藏其他应用的窗口，保留当前应用；其他应用的 UWP 外框也接收 `WM_CLOSE`。当前 UWP 应用身份无法区分时，跳过其他 UWP 外框。 |
| Alt + Tab | 临时恢复带有效 `MTU_PROC_TAG` 的可预览窗口，并交给 Windows 原生切换器。选中应用并松开 Alt 后，解封本轮同进程的预览窗口，其余窗口重新封印。Esc 取消本次预览。 |
| 点击主屏任务栏最右侧 30 像素区域 | 在最小化与恢复主屏普通窗口之间切换，跳过已封印窗口、副屏窗口、工具窗口及有 owner 的弹窗。 |
| 再次打开同应用窗口或通过关联文件唤醒 | 事件处理尝试恢复同进程的已封印窗口并清理标签。 |

程序在封印前保存窗口原始扩展样式、可读取的分层窗口透明度、可见性及窗口布局信息；无法安全读取原有分层属性的窗口会跳过封印。封印时通过透明度为 0、鼠标穿透、工具窗口样式及任务栏接口隐藏窗口，并用 `MTU_PROC_TAG` 记录进程名。窗口标题会添加蛛网前缀。预览时恢复原有的不透明度，临时添加 `WS_EX_APPWINDOW` 并显式显示窗口。原本就是工具窗口、最小化或隐藏的窗口不进入预览。最终解封会恢复原始扩展样式并清除标签和快照。`Win+C` 和 `Shift+Win+C` 向匹配的 UWP 外框发送常规关闭请求。

窗口仍留在所属桌面。原生 `Alt+Tab` 是否列出其他桌面的窗口取决于 Windows 的多任务设置。选择「仅限我正在使用的桌面」可以检查当前桌面的切换器列表。选中应用后，同进程的预览窗口也会在各自桌面解封。

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

测试位于独立的 `test/` 目录，通过 CTest 驱动 33 个用例，直接编译并调用 `LassoWindows.cpp` 的实际实现，不执行 `WinMain`，也不安装全局钩子。测试创建并销毁自己的 Win32 窗口；INI 临时文件只放在测试可执行文件旁，用例串行运行并自动清理。

```powershell
ctest --test-dir out/test -C Release --output-on-failure --no-tests=error --output-junit test-results.xml
./test/write_report.ps1
```

测试覆盖 INI 图标解析、进程识别、窗口过滤、坐标与可见性、隐藏和恢复样式与透明度，以及 `Win+C` 的进程标记、`Alt+Tab` 同进程解封、取消后重新封印和连续预览。

`.github/workflows/test_cpp.yml` 在 `windows-latest` 上构建应用与测试，响应 main 推送、指向 main 的 PR 和手动触发。参考 [xlsx2md](https://github.com/rarnhe/xlsx2md) 的报告方式：

- 每次运行将结果写入 Actions Summary，并上传 `test_report.md`、JUnit 和诊断日志为 artifact；配置/编译失败也会生成失败报告。
- main 推送运行会自动提交根目录 `test_report.md`，仅报告变化不会再次触发流水线。仓库需允许 Actions 写入内容；分支保护若禁止机器人直接推送，报告提交步骤会失败，artifact 仍保留。
- PR（含 fork）和手动运行通过 Summary/artifact 保存本次 Markdown 报告，不回写分支。仓库中的报告包含生成时的提交与运行来源，请据此判断是否对应当前代码。

自动测试不等于完整桌面端到端验证。全局快捷键、Alt+Tab 缩略图、UWP、任务栏视觉效果、双屏/混合 DPI、命名虚拟桌面切换与通知需在交互式桌面人工检查。

## 当前实现限制

- Win+Tab 的窗口转移仍为 TODO：当前只检测中文标题“任务视图”并显示提示，没有实现自动转移窗口。
- 已增加原始扩展样式、可读取透明度等快照及正常退出恢复；但 Windows 的特殊分层窗口效果、受权限保护的窗口和强制结束进程后的恢复仍无法保证，必须实机验证。
- 如果只识别到共享宿主，`Win+C` 只请求关闭前台 UWP 外框。应用可能显示保存提示或拒绝关闭。固定的任务栏启动图标由 Explorer 管理。实际按钮和 Alt+Tab 缩略图需要在交互式桌面验证。
