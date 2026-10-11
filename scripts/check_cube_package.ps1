param(
    [Parameter(Mandatory=$true)][string]$PackageRoot,
    [string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot),
    [Parameter(Mandatory=$true)][string]$ManifestPath
)
$ErrorActionPreference = 'Stop'
$cubeRoot = (Resolve-Path -LiteralPath $PackageRoot).Path
$sourceRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path
if ((Split-Path -Leaf $cubeRoot) -ne 'STM32Cube_FW_F4_V1.28.3') {
    throw 'Expected fixed package directory STM32Cube_FW_F4_V1.28.3'
}
$driverRoot = Join-Path $sourceRoot 'Drivers'
$rows = foreach ($file in Get-ChildItem -LiteralPath $driverRoot -Recurse -File) {
    $relative = $file.FullName.Substring($sourceRoot.Length + 1)
    $reference = Join-Path $cubeRoot $relative
    $actualHash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
    $expectedHash = if (Test-Path -LiteralPath $reference -PathType Leaf) {
        (Get-FileHash -LiteralPath $reference -Algorithm SHA256).Hash
    } else { '' }
    [pscustomobject]@{ Path=$relative; SHA256=$actualHash; PackageSHA256=$expectedHash; Match=($actualHash -eq $expectedHash) }
}
if (!$rows) { throw 'No dependency files found' }
$rows | Sort-Object Path | Export-Csv -LiteralPath $ManifestPath -NoTypeInformation -Encoding utf8
$bad = @($rows | Where-Object { !$_.Match })
Write-Output "Compared $($rows.Count) dependency files with local fixed package; mismatches=$($bad.Count)"
if ($bad.Count) { $bad | Select-Object Path; exit 1 }
