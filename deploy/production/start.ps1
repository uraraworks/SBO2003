# Sbo2003 サーバーを起動する（本番 VPS 用。タスクスケジューラの「SBO2003_SboSvr」から呼ばれる）。
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File C:\SBO2003\start.ps1
#
# 1. data\ が無ければ seed\ の元データをコピーする（初回だけ）
# 2. sbo2003.env の SBO2003_ADMIN_PASSWORD を SboSvr.ini の管理者パスワードにする
# 3. サーバー（node\node.exe server\SboSvr.js）を動かし続ける。
#    stop.ps1 で止めた時（data\stop.request がある時）はそのまま終わり、
#    それ以外で終わった（落ちた）時は 10 秒待ってから起動し直す。
# ログは log\ に起動ごとのファイルで残す（30 日より古いものは消す）。
$ErrorActionPreference = 'Stop'
$Root = $PSScriptRoot
$Data = Join-Path $Root 'data'
$LogDir = Join-Path $Root 'log'
$Node = Join-Path $Root 'node\node.exe'
$StopFile = Join-Path $Data 'stop.request'
$NoticeFile = Join-Path $Data 'notice.request'
$PidFile = Join-Path $Data 'sbosvr.pid'
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
$StartLog = Join-Path $LogDir 'start.log'

function Write-StartLog([string]$msg) {
	$line = '{0} {1}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $msg
	Write-Host $line
	Add-Content -Path $StartLog -Value $line -Encoding UTF8
}

# sbo2003.env を読む（「名前=値」の行だけ。値はそのまま使う）
$Conf = @{ SBO2003_HTTP_PORT = '18090'; SBO2003_ADMIN_PASSWORD = '' }
$EnvFile = Join-Path $Root 'sbo2003.env'
if (Test-Path $EnvFile) {
	foreach ($line in Get-Content -Path $EnvFile -Encoding UTF8) {
		if ($line -match '^\s*#' -or $line -notmatch '^\s*([A-Za-z0-9_]+)\s*=(.*)$') { continue }
		$Conf[$Matches[1]] = $Matches[2].Trim()
	}
} else {
	Write-StartLog '注意: sbo2003.env がありません（sbo2003.env.example をコピーして作成してください）'
}

# すでに動いていたら二重に起動しない
if (Test-Path $PidFile) {
	$old = Get-Process -Id ([int](Get-Content $PidFile -Raw)) -ErrorAction SilentlyContinue
	if ($old -and $old.Path -eq $Node) {
		Write-StartLog "すでに動いています（PID $($old.Id)）。起動しません"
		exit 0
	}
}

# 初回だけ元データをコピーする
if (-not (Test-Path (Join-Path $Data 'SBODATA'))) {
	Write-StartLog "データフォルダを作ります: $Data"
	New-Item -ItemType Directory -Force -Path $Data | Out-Null
	Copy-Item -Recurse (Join-Path $Root 'seed\SBODATA') $Data
}
# 手で持ってきたデータに SboSvr.ini が入っていなかった時も、元の設定で補う
if (-not (Test-Path (Join-Path $Data 'SboSvr.ini'))) {
	Write-StartLog 'SboSvr.ini が無いので元データのものを置きます'
	Copy-Item (Join-Path $Root 'seed\SboSvr.ini') (Join-Path $Data 'SboSvr.ini')
}

if ($Conf.SBO2003_ADMIN_PASSWORD) {
	& $Node (Join-Path $Root 'set-admin-password.js') (Join-Path $Data 'SboSvr.ini') $Conf.SBO2003_ADMIN_PASSWORD
	if ($LASTEXITCODE -ne 0) { throw '管理者パスワードを設定できませんでした' }
	Write-StartLog '管理者パスワードを設定しました'
} else {
	Write-StartLog '注意: SBO2003_ADMIN_PASSWORD が空です（管理者パスワード無しのまま）'
}

# 古いログを消す
Get-ChildItem -Path $LogDir -Filter 'sbosvr-*.log' |
	Where-Object { $_.LastWriteTime -lt (Get-Date).AddDays(-30) } |
	Remove-Item -Force -ErrorAction SilentlyContinue

# 前回の停止の印が残っていたら消す（残っていると起動してすぐ止まるため）
Remove-Item -Force -ErrorAction SilentlyContinue $StopFile
# サーバーはこのファイルが置かれたら保存して終わる（server/node/sbosvr_net.js）
$env:SBO2003_STOP_FILE = $StopFile
# このファイルが置かれたら、中身をゲームの全員にお知らせとして流す（remote-deploy.ps1 の再起動予告）
Remove-Item -Force -ErrorAction SilentlyContinue $NoticeFile
$env:SBO2003_NOTICE_FILE = $NoticeFile

$NodeArgs = '"{0}" --headless --data "{1}" --http-port {2} --www "{3}"' -f `
	(Join-Path $Root 'server\SboSvr.js'), $Data, $Conf.SBO2003_HTTP_PORT, (Join-Path $Root 'web')
while ($true) {
	$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
	$out = Join-Path $LogDir "sbosvr-$stamp.out.log"
	$err = Join-Path $LogDir "sbosvr-$stamp.err.log"
	$p = Start-Process -FilePath $Node -ArgumentList $NodeArgs -WorkingDirectory $Root -NoNewWindow -PassThru `
		-RedirectStandardOutput $out -RedirectStandardError $err
	# 終了コードを後で読めるよう、ハンドルを先に取っておく
	$null = $p.Handle
	Set-Content -Path $PidFile -Value $p.Id -Encoding ASCII
	Write-StartLog "サーバーを起動しました（PID $($p.Id)、HTTP ポート $($Conf.SBO2003_HTTP_PORT)、ログ $out）"
	$p.WaitForExit()
	Remove-Item -Force -ErrorAction SilentlyContinue $PidFile
	if (Test-Path $StopFile) {
		Remove-Item -Force -ErrorAction SilentlyContinue $StopFile
		Write-StartLog "サーバーが止まりました（終了コード $($p.ExitCode)、stop.ps1 による停止）"
		break
	}
	Write-StartLog "サーバーが終わってしまいました（終了コード $($p.ExitCode)）。10 秒後に起動し直します"
	Start-Sleep -Seconds 10
}
