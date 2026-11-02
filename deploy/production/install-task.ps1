# Sbo2003 サーバーを Windows の起動時に自動で立ち上げるタスク「SBO2003_SboSvr」を登録する（初回だけ。管理者で実行）。
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File C:\SBO2003\install-task.ps1
#
# - 起動時（ログオン不要）に SYSTEM で start.ps1 を動かす。VPS が再起動してもサーバーが自動で戻る
# - サーバーが落ちた時は start.ps1 が起動し直す。start.ps1 自体が失敗した時はタスクが 1 分おきに 3 回までやり直す
# - 実行時間の上限なし（既定の 3 日で止められないように）
# もう一度実行すると、同じ名前のタスクを作り直す（設定を変えた時など）。
$ErrorActionPreference = 'Stop'
$TaskName = 'SBO2003_SboSvr'
$Start = Join-Path $PSScriptRoot 'start.ps1'

$principal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
	throw '管理者の PowerShell で実行してください'
}

$action = New-ScheduledTaskAction -Execute 'powershell.exe' `
	-Argument ('-NoProfile -ExecutionPolicy Bypass -File "{0}"' -f $Start) -WorkingDirectory $PSScriptRoot
$trigger = New-ScheduledTaskTrigger -AtStartup
$settings = New-ScheduledTaskSettingsSet -ExecutionTimeLimit ([TimeSpan]::Zero) `
	-RestartCount 3 -RestartInterval (New-TimeSpan -Minutes 1) `
	-MultipleInstances IgnoreNew -StartWhenAvailable -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries
$user = New-ScheduledTaskPrincipal -UserId 'SYSTEM' -LogonType ServiceAccount -RunLevel Highest

Register-ScheduledTask -TaskName $TaskName -Action $action -Trigger $trigger -Settings $settings `
	-Principal $user -Description 'Sbo2003 サーバー（start.ps1）。止める時は stop.ps1 を使う' -Force | Out-Null
Write-Host "タスク「$TaskName」を登録しました。今すぐ起動する時は: Start-ScheduledTask -TaskName $TaskName"
