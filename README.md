# LassoWindows

LassoWindows 是基于 Win32、COM 与 C++/WinRT 的后台窗口管理工具。程序通过键盘、鼠标及窗口事件钩子，按应用进程和当前虚拟桌面管理窗口。以下说明基于 `LassoWindows.cpp` 当前实现。

## 功能与操作

| 操作 | 当前行为 |
| --- | --- |
| Win + C | 隐藏当前应用在当前虚拟桌面的同进程窗口；Explorer 仅处理文件夹窗口。对于 `ApplicationFrameWindow` 托管的 UWP 窗口，代码发送 `WM_CLOSE`，实际是关闭而非隐藏。 |
| Shift + Win + C | 尝试隐藏当前桌面中其他应用的窗口，保留当前应用；UWP 分支存在已知问题，见下文。 |
| Alt + Tab | 临时恢复本桌面被标记的窗口供切换器预览；Alt 释放后由 150ms 定时器处理保留/收网，Esc 触发重新隐藏。 |
| 点击主屏任务栏最右侧 30 像素区域 | 在最小化与恢复主屏普通窗口之间切换，跳过已封印窗口、副屏窗口、工具窗口及有 owner 的弹窗。 |
| 再次打开同应用窗口或通过关联文件唤醒 | 事件处理尝试恢复同进程、同桌面的已隐藏窗口并清理标签。 |

程序通过透明度为 0、鼠标穿透、工具窗口样式及任务栏接口隐藏窗口，并使用 `MTU_PROC_TAG` / `MTU_VD_TAG` Atom 属性记录进程名和桌面名称，标题添加蛛网前缀。恢复时撤销相关标签与样式。

任务栏相关代码会备份/恢复 AppUserModelID，缓存原始图标，并尝试设置窗口快照与叠加图标。启动时以命名互斥量限制单实例，初始化 WinRT，尝试创建开始菜单快捷方式，然后进入消息循环；退出路径解除钩子并释放资源。程序目前没有设置界面或托盘退出入口，必要时可在任务管理器结束进程。

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

测试位于独立的 `test/` 目录，通过 CTest 驱动 18 个用例，直接编译并调用 `LassoWindows.cpp` 的实际实现，不执行 `WinMain`，也不安装全局钩子。测试创建并销毁自己的 Win32 窗口；INI 临时文件只放在测试可执行文件旁，用例串行运行并自动清理。

```powershell
ctest --test-dir out/test -C Release --output-on-failure --no-tests=error --output-junit test-results.xml
./test/write_report.ps1
```

覆盖 INI 正常/缺失/无效索引/含逗号路径、进程识别、窗口过滤、坐标与可见性、隐藏/恢复样式与透明度、压制条件、收网状态、图标缓存和主屏收集过滤。

`.github/workflows/test_cpp.yml` 在 `windows-latest` 上构建应用与测试，响应 main 推送、指向 main 的 PR 和手动触发。参考 [xlsx2md](https://github.com/rarnhe/xlsx2md) 的报告方式：

- 每次运行将结果写入 Actions Summary，并上传 `test_report.md`、JUnit 和诊断日志为 artifact；配置/编译失败也会生成失败报告。
- main 推送运行会自动提交根目录 `test_report.md`，仅报告变化不会再次触发流水线。仓库需允许 Actions 写入内容；分支保护若禁止机器人直接推送，报告提交步骤会失败，artifact 仍保留。
- PR（含 fork）和手动运行通过 Summary/artifact 保存本次 Markdown 报告，不回写分支。仓库中的报告包含生成时的提交与运行来源，请据此判断是否对应当前代码。

自动测试不等于完整桌面端到端验证。全局快捷键、Alt+Tab 缩略图、UWP、任务栏视觉效果、双屏/混合 DPI、命名虚拟桌面切换与通知需在交互式桌面人工检查。

## 当前实现限制

- Win+Tab 的窗口转移仍为 TODO：当前只检测中文标题“任务视图”并显示提示，没有实现自动转移窗口。
- 虚拟桌面名称通过 Explorer 注册表读取；未命名或读取失败时返回空字符串并尝试通知，按桌面名称匹配的恢复逻辑可能无法工作。
- `EnumHideOthersProc` 将同一参数同时按进程名字符串与 `CloseFilter` 解读；Shift+Win+C 的 UWP 路径存在类型不匹配风险，尚未修复或纳入自动测试。
- 桌面切换事件钩子在启动中注册后立即解除；不要将自动随桌面切换视为已完成的功能。
- 隐藏/恢复函数没有保存任意应用原有的全部扩展样式；带自定义透明/工具窗口样式的应用需要单独验证。进程退出也没有批量恢复所有已封印窗口的逻辑。
