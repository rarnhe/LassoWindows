param(
    [string]$BuildDirectory = "out/test",
    [string]$Configure = "success",
    [string]$Build = "success",
    [string]$Test = "success",
    [string]$OutputPath = "test_report.md",
    [string]$LogDirectory = ""
)
$ErrorActionPreference = "Stop"
$cases = @()
$junitError = ""
$junit = Join-Path $BuildDirectory "test-results.xml"
if (Test-Path -LiteralPath $junit) {
    try {
        [xml]$document = Get-Content -LiteralPath $junit -Raw
        $cases = @($document.SelectNodes("//testcase"))
    } catch {
        $junitError = $_.Exception.Message
    }
}
$failed = @($cases | Where-Object { $_.SelectSingleNode("failure") -or $_.SelectSingleNode("error") })
$skipped = @($cases | Where-Object { $_.SelectSingleNode("skipped") })
$passed = $cases.Count - $failed.Count - $skipped.Count
$ok = $Configure -eq "success" -and $Build -eq "success" -and $Test -eq "success" -and $cases.Count -gt 0 -and $failed.Count -eq 0 -and $skipped.Count -eq 0
$status = if ($ok) { "通过 (Passed)" } else { "失败或未完整执行 (Failed)" }
$revision = git rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw "Cannot read tested revision" }
$run = if ($env:GITHUB_RUN_ID) { "$env:GITHUB_SERVER_URL/$env:GITHUB_REPOSITORY/actions/runs/$env:GITHUB_RUN_ID" } else { "本地 Windows 执行（含当前工作区测试改动）" }
$lines = [System.Collections.Generic.List[string]]::new()
@(
    "# C++ Windows 测试报告", "",
    "- 状态：$status",
    "- 时间（UTC）：$([DateTime]::UtcNow.ToString('yyyy-MM-dd HH:mm:ss'))",
    "- 测试源码基准提交：$revision",
    "- 执行记录：$run",
    "- 平台：Windows / MSVC / C++20 / Win32 + Windows SDK",
    "- 阶段：Configure=$Configure / Build=$Build / CTest=$Test",
    "- 统计：通过 $passed / 失败 $($failed.Count) / 跳过 $($skipped.Count) / 共 $($cases.Count)", "",
    "| 测试 | 结果 |", "| --- | --- |"
) | ForEach-Object { $lines.Add($_) }
foreach ($case in $cases) {
    $outcome = if ($case.SelectSingleNode("failure") -or $case.SelectSingleNode("error")) { "失败" } elseif ($case.SelectSingleNode("skipped")) { "跳过" } else { "通过" }
    $lines.Add("| $($case.name) | $outcome |")
}
if ($cases.Count -eq 0) { $lines.Add("| 未获得 CTest JUnit 结果，请查看配置/编译日志 | 未执行 |") }
$lines.Add("")
$lines.Add("## 失败详情")
foreach ($case in $failed) {
    $lines.Add("")
    $lines.Add("### $($case.name)")
    $lines.Add("<pre>")
    $lines.Add([System.Net.WebUtility]::HtmlEncode($case.InnerText))
    $lines.Add("</pre>")
}
if ($junitError) {
    $lines.Add("无法读取 CTest JUnit 结果。")
    $lines.Add("<pre>$([System.Net.WebUtility]::HtmlEncode($junitError))</pre>")
}
if ($LogDirectory) {
    $phases = [ordered]@{ configure = $Configure; build = $Build; ctest = $Test }
    foreach ($phase in $phases.Keys) {
        if ($phases[$phase] -ne "failure") { continue }
        $log = Join-Path $LogDirectory "$phase.log"
        if (!(Test-Path -LiteralPath $log)) { continue }
        $output = (Get-Content -LiteralPath $log -Tail 80) -join [Environment]::NewLine
        $lines.Add("")
        $lines.Add("### $phase 最后 80 行输出")
        $lines.Add("<pre>$([System.Net.WebUtility]::HtmlEncode($output))</pre>")
    }
}
if ($ok) { $lines.Add("无。") }
$lines.Add("")
$lines.Add("## 覆盖范围")
$lines.Add("真实编译 LassoWindows.cpp，验证 INI 图标解析、EXE 文件名与虚拟桌面 GUID 双标签、跨虚拟桌面同名 EXE 封印、Shift+Win+C 跨全部虚拟桌面隐藏其他应用及 UWP WM_CLOSE、会话级虚拟桌面 GUID 优先解析、切换桌面后 Alt+Tab 仅预览当前桌面窗口、主屏原生 Show Desktop 按钮精确命中、主屏窗口透明化及还原、桌面图标点击的恢复规则、任务栏图标样式保留、同进程解封、连续预览、收网计时器隔离、图标缓存和主屏筛选条件。测试仅操作自己创建的窗口，不启动 WinMain 或安装全局钩子。")
$lines.Add("")
$lines.Add("全局快捷键、Alt+Tab 实际缩略图、Explorer/UWP、任务栏视觉效果、多显示器/不同 DPI、虚拟桌面切换与通知仍需交互式人工验证；这些项目未计入自动测试通过数。")
$content = $lines -join [Environment]::NewLine
Set-Content -LiteralPath $OutputPath -Value $content -Encoding utf8
if ($env:GITHUB_STEP_SUMMARY) { Add-Content -LiteralPath $env:GITHUB_STEP_SUMMARY -Value $content -Encoding utf8 }
