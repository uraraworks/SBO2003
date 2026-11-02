# Sbo2003 サーバーを止める（データを保存してから止まる。本番 VPS 用）。
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File C:\SBO2003\stop.ps1
#
# data\stop.request を置くと、サーバーはデータを保存して終わり、start.ps1 も起動し直さずに終わる。
# Stop-Process や Stop-ScheduledTask でいきなり止めると保存されないので、必ずこちらで止める。
param(
	# 保存が終わるのを待つ秒数。過ぎても止まらない時は強制終了する
	[int]$TimeoutSec = 120
)
$ErrorActionPreference = 'Stop'
$Root = $PSScriptRoot
$Data = Join-Path $Root 'data'
$StopFile = Join-Path $Data 'stop.request'
$PidFile = Join-Path $Data 'sbosvr.pid'

$p = $null
if (Test-Path $PidFile) {
	$p = Get-Process -Id ([int](Get-Content $PidFile -Raw)) -ErrorAction SilentlyContinue
}
if (-not $p) {
	Write-Host 'サーバーは動いていません'
	exit 0
}

Write-Host "サーバー（PID $($p.Id)）にデータを保存して止まるよう頼みます"
Set-Content -Path $StopFile -Value (Get-Date -Format 'yyyy-MM-dd HH:mm:ss') -Encoding ASCII
if ($p.WaitForExit($TimeoutSec * 1000)) {
	Write-Host 'サーバーが止まりました'
} else {
	Write-Warning "$TimeoutSec 秒待っても止まらないので強制終了します（最後の保存のあとのデータは失われます）"
	Stop-Process -Id $p.Id -Force
}

# start.ps1 が終わるのを少し待つ（タスクの状態が「準備完了」に戻る）
$task = Get-ScheduledTask -TaskName 'SBO2003_SboSvr' -ErrorAction SilentlyContinue
if ($task) {
	for ($i = 0; $i -lt 15 -and (Get-ScheduledTask -TaskName 'SBO2003_SboSvr').State -eq 'Running'; $i++) {
		Start-Sleep -Seconds 1
	}
}
