# Sbo2003 本番 VPS での入れ替え（SSH 越しに vps-deploy.sh から呼ばれる。手で動かしてもよい）。管理者で実行する。
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File C:\SBO2003\upload\remote-deploy.ps1 -Zip C:\SBO2003\upload\Sbo2003_Server_xxx.zip
#   powershell ... remote-deploy.ps1 -Data C:\SBO2003\upload\sbo2003-data.zip   # データだけ入れ替える
#   powershell ... remote-deploy.ps1 -Rollback                                  # 1 つ前の版に戻す
#
# 1. stop.ps1 でデータを保存してからサーバーを止める
# 2. -Zip: 今の server\ web\ node\ seed\ を prev\ に退避して、新しい zip を展開する
#    -Data: 今の data\ を backup\ に zip で控えてから、渡されたデータ（SBODATA/ と SboSvr.ini の zip）に置き換える
#    -Rollback: prev\ と今の版を入れ替える（data\ はそのまま）
# 3. 初回はタスク SBO2003_SboSvr を登録し、sbo2003.env が無ければ雛形から作る
# 4. タスクを起動して /health が ok を返すまで待つ
# data\・log\・sbo2003.env は zip に入っていないので、そのまま残る。
param(
	[string]$Zip = '',
	[string]$Data = '',
	[switch]$Rollback,
	[string]$Root = 'C:\SBO2003'
)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
# SSH 越しでも日本語が化けないよう、出力を UTF-8 にする
[Console]::OutputEncoding = [Text.Encoding]::UTF8
$TaskName = 'SBO2003_SboSvr'
$Parts = @('server', 'web', 'node', 'seed')

function Say([string]$msg) { Write-Output ('[{0}] {1}' -f (Get-Date -Format 'HH:mm:ss'), $msg) }

$principal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
	throw '管理者の権限で実行してください（SSH のユーザーが Administrators に入っていること）'
}
if (-not $Zip -and -not $Data -and -not $Rollback) { throw '-Zip・-Data・-Rollback のどれかを指定してください' }
if ($Zip -and -not (Test-Path $Zip)) { throw "zip がありません: $Zip" }
if ($Data -and -not (Test-Path $Data)) { throw "データがありません: $Data" }
New-Item -ItemType Directory -Force -Path $Root | Out-Null

# 1. 止める（保存してから）
$stop = Join-Path $Root 'stop.ps1'
if (Test-Path $stop) {
	Say 'サーバーを止めます（データを保存してから）'
	& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $stop
	if ($LASTEXITCODE -ne 0) { throw "stop.ps1 が失敗しました（$LASTEXITCODE）" }
}

# 2-a. 版の入れ替え
if ($Rollback) {
	$prev = Join-Path $Root 'prev'
	if (-not (Test-Path (Join-Path $prev 'server'))) { throw '戻せる版（prev\）がありません' }
	Say '1 つ前の版に戻します'
	$swap = Join-Path $Root 'prev.swap'
	if (Test-Path $swap) { Remove-Item $swap -Recurse -Force }
	New-Item -ItemType Directory -Path $swap | Out-Null
	foreach ($p in $Parts) {
		$cur = Join-Path $Root $p
		if (Test-Path $cur) { Move-Item $cur (Join-Path $swap $p) }
		if (Test-Path (Join-Path $prev $p)) { Move-Item (Join-Path $prev $p) $cur }
	}
	Remove-Item $prev -Recurse -Force
	Rename-Item $swap 'prev'
} elseif ($Zip) {
	$prev = Join-Path $Root 'prev'
	if (Test-Path (Join-Path $Root 'server')) {
		Say '今の版を prev\ に退避します'
		if (Test-Path $prev) { Remove-Item $prev -Recurse -Force }
		New-Item -ItemType Directory -Path $prev | Out-Null
		foreach ($p in $Parts) {
			$cur = Join-Path $Root $p
			if (Test-Path $cur) { Move-Item $cur (Join-Path $prev $p) }
		}
	}
	Say "展開します: $(Split-Path $Zip -Leaf)"
	Expand-Archive -Path $Zip -DestinationPath $Root -Force
	Remove-Item $Zip -Force
}

# 2-b. データの入れ替え
if ($Data) {
	$dataDir = Join-Path $Root 'data'
	if (Test-Path (Join-Path $dataDir 'SBODATA')) {
		$backupDir = Join-Path $Root 'backup'
		New-Item -ItemType Directory -Force -Path $backupDir | Out-Null
		$backup = Join-Path $backupDir ('sbo2003-data-{0}.zip' -f (Get-Date -Format 'yyyyMMdd_HHmmss'))
		Say "今のデータを控えます: $backup"
		Compress-Archive -Path (Join-Path $dataDir '*') -DestinationPath $backup
	}
	$work = Join-Path $Root 'upload\data.new'
	if (Test-Path $work) { Remove-Item $work -Recurse -Force }
	New-Item -ItemType Directory -Path $work | Out-Null
	Expand-Archive -Path $Data -DestinationPath $work -Force
	if (-not (Test-Path (Join-Path $work 'SBODATA'))) { throw '渡されたデータに SBODATA\ がありません' }
	Say 'データを入れ替えます'
	New-Item -ItemType Directory -Force -Path $dataDir | Out-Null
	foreach ($name in @('SBODATA', 'SboSvr.ini')) {
		$dst = Join-Path $dataDir $name
		if (Test-Path $dst) { Remove-Item $dst -Recurse -Force }
		if (Test-Path (Join-Path $work $name)) { Move-Item (Join-Path $work $name) $dst }
	}
	Remove-Item $work -Recurse -Force
	Remove-Item $Data -Force
}

# 3. 初回だけの準備
$envFile = Join-Path $Root 'sbo2003.env'
if (-not (Test-Path $envFile)) {
	Copy-Item (Join-Path $Root 'sbo2003.env.example') $envFile
	Write-Warning "sbo2003.env を雛形から作りました。SBO2003_ADMIN_PASSWORD を書いてから stop.ps1 → 起動し直してください（空のままだと管理者ではログインできません）"
}
if (-not (Get-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue)) {
	Say "タスク $TaskName を登録します"
	& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root 'install-task.ps1')
	if ($LASTEXITCODE -ne 0) { throw "install-task.ps1 が失敗しました（$LASTEXITCODE）" }
}

# 4. 起動して確かめる
$port = '18090'
foreach ($line in Get-Content -Path $envFile -Encoding UTF8) {
	if ($line -match '^\s*SBO2003_HTTP_PORT\s*=\s*(\d+)') { $port = $Matches[1] }
}
Say 'サーバーを起動します'
Start-ScheduledTask -TaskName $TaskName
$ok = $false
for ($i = 0; $i -lt 60 -and -not $ok; $i++) {
	Start-Sleep -Seconds 2
	try { $ok = (Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/health" -TimeoutSec 5).Content -match 'ok' } catch { }
}
if (-not $ok) {
	Get-Content (Join-Path $Root 'log\start.log') -Tail 20 -ErrorAction SilentlyContinue
	throw "/health（127.0.0.1:$port）に応答がありません。-Rollback で 1 つ前の版に戻せます"
}
Say '起動しました（/health が ok）'
