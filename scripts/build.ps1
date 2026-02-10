param(
  [ValidateSet('windows', 'web', 'native', 'wsl-native')]
  [string]$Preset = 'windows',
  [ValidateSet('Debug', 'Release')]
  [string]$Config,
  [switch]$CleanFirst,
  [switch]$Parallel,
  [switch]$Time
)
$cmake = "cmake" 
if ($Preset -eq 'wsl-native') { 
  $cmake = "wsl cmake"
  $Preset = "native"
}
$flags = (
  ($Config, "--config $Config"),
  ($CleanFirst, '--clean-first'),
  ($Parallel, '--parallel')`
| Where-Object { $PSItem[0] } | ForEach-Object { $PSItem[1] }) -join ' '
$outer_sw = [System.Diagnostics.Stopwatch]::StartNew()

"$cmake --preset $Preset",
"$cmake --build --preset $Preset"`

| ForEach-Object {
  $command = $PSItem
  if ($command -like '*--build*') {
    $command = "$command $flags"
  }
  $sw = [System.Diagnostics.Stopwatch]::StartNew()

  "`n`n>> $command" 
  Invoke-Expression "$command" 
  if ($LASTEXITCODE -ne 0) {
    "`"$command`" failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
  }

  $sw.Stop()
  if ($Time) { "`nCommand Time $($sw.Elapsed)" }
}

$outer_sw.Stop()
if ($Time) { "`Total Time $($outer_sw.Elapsed)" }