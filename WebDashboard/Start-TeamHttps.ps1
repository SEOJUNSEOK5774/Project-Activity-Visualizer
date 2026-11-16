param(
    [Parameter(Mandatory = $true)]
    [string]$ServerIp
)

$ErrorActionPreference = 'Stop'

$Address = $null
if ($ServerIp -notmatch '^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$' -or
    -not [System.Net.IPAddress]::TryParse($ServerIp, [ref]$Address) -or
    $Address.ToString() -ne $ServerIp) {
    throw 'Enter the IPv4 address shown by ipconfig, such as 192.168.1.20.'
}
$Octets = $Address.GetAddressBytes()
$IsPrivate = $Octets[0] -in 10, 127 -or
    ($Octets[0] -eq 172 -and $Octets[1] -ge 16 -and $Octets[1] -le 31) -or
    ($Octets[0] -eq 192 -and $Octets[1] -eq 168)
if (-not $IsPrivate) {
    throw 'Use a private LAN IPv4 address or 127.0.0.1 for a local check.'
}

$AccountsFile = Join-Path $PSScriptRoot 'team-users.caddy'
if (-not (Test-Path -LiteralPath $AccountsFile -PathType Leaf)) {
    throw 'Create team-users.caddy using the account setup instructions in TEAM-HTTPS.md.'
}
$AccountLines = @(Get-Content -LiteralPath $AccountsFile |
    Where-Object { $_ -notmatch '^\s*(?:#.*)?$' })
if ($AccountLines.Count -eq 0) {
    throw 'Add at least one team account before starting HTTPS.'
}
$Usernames = @{}
foreach ($Line in $AccountLines) {
    if ($Line -notmatch '^\s*([a-zA-Z0-9_.-]+)\s+\$2[aby]\$[0-9]{2}\$[a-zA-Z0-9./]{53}\s*$') {
        throw 'Each account line must contain a username and a bcrypt hash from caddy hash-password.'
    }
    $Username = $Matches[1]
    if ($Usernames.ContainsKey($Username)) {
        throw 'Use a different username for each account.'
    }
    $Usernames[$Username] = $true
}

$Caddy = Join-Path $PSScriptRoot 'caddy.exe'
if (-not (Test-Path -LiteralPath $Caddy -PathType Leaf)) {
    $Caddy = (Get-Command caddy.exe -CommandType Application -ErrorAction Stop).Source
}

$PreviousAddress = $env:AV_TEAM_IP
Push-Location -LiteralPath $PSScriptRoot
try {
    $env:AV_TEAM_IP = $ServerIp
    & $Caddy validate --config Caddyfile --adapter caddyfile
    if ($LASTEXITCODE -ne 0) {
        throw 'Caddy could not validate Caddyfile.'
    }
    Write-Host "Team dashboard: https://${ServerIp}:8443/"
    & $Caddy run --config Caddyfile --adapter caddyfile
    if ($LASTEXITCODE -ne 0) {
        throw 'Caddy could not start. See its error above.'
    }
}
finally {
    $env:AV_TEAM_IP = $PreviousAddress
    Pop-Location
}
