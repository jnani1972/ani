$ErrorActionPreference = 'Stop'

$packageName = 'ani'
$installDir  = Join-Path $env:ChocolateyBinRoot $packageName

Uninstall-BinFile -Name 'ani'

if (Test-Path $installDir) {
  Remove-Item $installDir -Recurse -Force
}
