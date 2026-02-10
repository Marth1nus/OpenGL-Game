param(
  [switch]$CleanFirst,
  [switch]$Parallel,
  [switch]$Time)
$flags = (
  ($CleanFirst, '--clean-first'),
  ($Parallel, '--parallel')`
  | Where-Object { $_[0] } | ForEach-Object { $_[1] }) -join ' '
$outer_sw = [System.Diagnostics.Stopwatch]::StartNew()
    
"cmake --preset windows",
"cmake --build --preset windows --config Debug",
"cmake --build --preset windows --config Release",

"cmake --preset web",
"cmake --build --preset web --config Debug",
"cmake --build --preset web --config Release",

"wsl cmake --preset native",
"wsl cmake --build --preset native --config Debug",
"wsl cmake --build --preset native --config Release"`

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
if ($Time) { "`nTotal Time $($outer_sw.Elapsed)" }

