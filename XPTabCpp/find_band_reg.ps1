Write-Output "=== Toolbar 下的 XPTab ==="
foreach ($p in @("HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar","HKCU:\Software\Microsoft\Internet Explorer\Toolbar")) {
  if (Test-Path $p) {
    $props = Get-ItemProperty $p
    $props.PSObject.Properties | Where-Object { $_.Value -match "XPTab" -or $_.Name -match "{.*}" } | ForEach-Object { Write-Output "$p -> $($_.Name) = $($_.Value)" }
  }
}

Write-Output "`n=== CLSID 中名为 XPTab 的项 ==="
Get-ChildItem "HKLM:\SOFTWARE\Classes\CLSID","HKCU:\Software\Classes\CLSID" -ErrorAction SilentlyContinue | ForEach-Object {
  $val = (Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue).'(default)'
  if ($val -match "XPTab") { Write-Output "$($_.PSPath) -> $val" }
}
