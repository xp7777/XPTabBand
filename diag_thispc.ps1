Write-Host "=== 检查桌面'此电脑'图标的注册表 Open 命令 ==="
$clsid = "{20D04FE0-3AEA-1069-A2D8-08002B30309D}"

$paths = @(
    "HKCR:\CLSID\$clsid\Shell\Open\Command",
    "HKCR:\CLSID\$clsid\Shell\OpenNewWindow\Command",
    "HKCU:\Software\Classes\CLSID\$clsid\Shell\Open\Command",
    "HKCU:\Software\Classes\CLSID\$clsid\Shell\OpenNewWindow\Command",
    "HKCR:\Folder\shell\open\command",
    "HKCU:\Software\Classes\Folder\shell\open\command"
)
foreach ($p in $paths) {
    if (Test-Path $p) {
        Write-Host "[$p] 存在:"
        $val = (Get-ItemProperty $p -ErrorAction SilentlyContinue)
        $val | Format-List
    } else {
        Write-Host "[$p] 不存在"
    }
}

Write-Host ""
Write-Host "=== 检查 Folder\shell\open 默认值 ==="
$openPath = "HKCR:\Folder\shell\open"
if (Test-Path $openPath) {
    $def = (Get-Item $openPath).GetValue("")
    Write-Host "默认值: $def"
    $sub = Get-ChildItem $openPath -ErrorAction SilentlyContinue
    Write-Host "子项:"
    $sub | ForEach-Object { Write-Host "  $($_.PSChildName)" }
}

Write-Host ""
Write-Host "=== 检查 explorer.exe LaunchExplorer / BrowseInPlace ==="
$root = "HKCR:\CLSID\$clsid"
if (Test-Path $root) {
    $def = (Get-Item $root).GetValue("")
    Write-Host "CLSID 默认值: $def"
    $shell = Get-ChildItem "$root\Shell" -ErrorAction SilentlyContinue
    if ($shell) {
        Write-Host "Shell 子项:"
        $shell | ForEach-Object { Write-Host "  $($_.PSChildName)" }
    } else {
        Write-Host "Shell 子项不存在"
    }
}

Write-Host ""
Write-Host "=== 检查 explorer.exe 进程命令行 ==="
Get-Process explorer | Select-Object Id, ProcessName | Format-Table

Write-Host ""
Write-Host "=== 测试用 explorer.exe 直接打开此电脑 ==="
Write-Host "执行: explorer.exe ::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"
Start-Process explorer.exe -ArgumentList "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"
Start-Sleep -Seconds 2
Write-Host "执行完毕，请观察是否打开了'此电脑'窗口"

Write-Host ""
Write-Host "=== 检查是否有 explorer.exe 子进程异常 ==="
$explorerProcs = Get-Process explorer
Write-Host "explorer 进程数: $($explorerProcs.Count)"
