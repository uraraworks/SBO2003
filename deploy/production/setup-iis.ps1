# Sbo2003 用の IIS サイトを作る（VPS で 1 回だけ。管理者で実行）。
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File C:\SBO2003\setup-iis.ps1 -HostName sbo2003.urara-works.jp
#
# - サイト「SBO2003」（物理パス C:\SBO2003\iis、アプリプール SBO2003）を作り、http（80）をホスト名で受ける
# - 証明書ストア（LocalMachine\My）にこのホスト名に使える証明書（ワイルドカード含む）があれば、https（443・SNI）も付ける
#   無ければ http だけ作って、証明書の取り方を表示する（SBOP2 のサイトと同じやり方で取る）
# - 既存のサイト（SBOP2 など）・IIS 全体の設定には触らない。もう一度実行しても、足りないものだけ足す
param(
	[Parameter(Mandatory = $true)][string]$HostName,
	[string]$Root = 'C:\SBO2003',
	[string]$SiteName = 'SBO2003'
)
$ErrorActionPreference = 'Stop'
Import-Module WebAdministration

$principal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
	throw '管理者の PowerShell で実行してください'
}
$iisDir = Join-Path $Root 'iis'
if (-not (Test-Path (Join-Path $iisDir 'web.config'))) { throw "$iisDir\web.config がありません（先に zip を展開してください）" }

# 逆プロキシに要るもの（SBOP2 で用意済みのはず）
if (-not (Get-WebGlobalModule -Name 'RewriteModule' -ErrorAction SilentlyContinue)) {
	Write-Warning 'URL Rewrite が入っていません（SBOP2 と同じものを入れてください）'
}
if (-not (Get-WebGlobalModule -Name 'ApplicationRequestRouting' -ErrorAction SilentlyContinue)) {
	Write-Warning 'Application Request Routing（ARR）が入っていません'
} elseif (-not (Get-WebConfigurationProperty -PSPath 'MACHINE/WEBROOT/APPHOST' -Filter 'system.webServer/proxy' -Name enabled).Value) {
	Write-Warning 'ARR のプロキシが無効です（IIS マネージャーのサーバーノード → Application Request Routing Cache → Server Proxy Settings → Enable proxy）'
}

# アプリプール（プロキシするだけなので .NET は使わない）
if (-not (Test-Path "IIS:\AppPools\$SiteName")) {
	New-WebAppPool -Name $SiteName | Out-Null
	Set-ItemProperty "IIS:\AppPools\$SiteName" -Name managedRuntimeVersion -Value ''
	Write-Output "アプリプール $SiteName を作りました"
}

# サイトと http のバインド
if (-not (Get-Website -Name $SiteName)) {
	New-Website -Name $SiteName -PhysicalPath $iisDir -ApplicationPool $SiteName -HostHeader $HostName -Port 80 | Out-Null
	Write-Output "サイト $SiteName を作りました（http://$HostName/）"
} elseif (-not (Get-WebBinding -Name $SiteName -Protocol http -HostHeader $HostName)) {
	New-WebBinding -Name $SiteName -Protocol http -Port 80 -HostHeader $HostName
	Write-Output "http のバインドを足しました"
}

# https: ホスト名に合う証明書を探す（期限の長いものを選ぶ）
if (Get-WebBinding -Name $SiteName -Protocol https -HostHeader $HostName) {
	Write-Output 'https のバインドはすでにあります'
} else {
	$wild = '*.' + ($HostName -replace '^[^.]+\.', '')
	$cert = Get-ChildItem Cert:\LocalMachine\My |
		Where-Object { $_.NotAfter -gt (Get-Date) -and $_.HasPrivateKey -and
			($_.DnsNameList.Unicode -contains $HostName -or $_.DnsNameList.Unicode -contains $wild) } |
		Sort-Object NotAfter -Descending | Select-Object -First 1
	if ($cert) {
		New-WebBinding -Name $SiteName -Protocol https -Port 443 -HostHeader $HostName -SslFlags 1
		(Get-WebBinding -Name $SiteName -Protocol https -HostHeader $HostName).AddSslCertificate($cert.Thumbprint, 'My')
		Write-Output "https のバインドを足しました（証明書 $($cert.Subject)、期限 $($cert.NotAfter.ToString('yyyy-MM-dd'))）"
	} else {
		Write-Warning "$HostName に使える証明書がありません。SBOP2 のサイトと同じやり方（win-acme など）で取り、もう一度このスクリプトを実行してください"
		Write-Output '参考: 今ある https のサイトと証明書'
		Get-ChildItem IIS:\SslBindings | Select-Object Host, Port, @{ n = 'Subject'; e = { (Get-Item "Cert:\LocalMachine\$($_.Store)\$($_.Thumbprint)" -ErrorAction SilentlyContinue).Subject } } | Format-Table -AutoSize
	}
}
Write-Output "確認: curl.exe -s http://127.0.0.1:18090/health が ok なら、https://$HostName/ でログイン画面が出ます"
