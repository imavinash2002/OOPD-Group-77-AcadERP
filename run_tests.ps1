# group-77
# Run from Windows PowerShell in a disposable sandbox, preserving the user's audit.log.
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$binary = Join-Path $PSScriptRoot 'acaderp.exe'
if (-not (Test-Path $binary)) {
    throw 'Compile acaderp.exe with the command from README.md first.'
}
$count = 0
$passed = 0
$sandbox = Join-Path $env:TEMP ('acaderp_test_' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path (Join-Path $sandbox 'tests') -Force | Out-Null
Copy-Item '.\tests\corrupt.save' (Join-Path $sandbox 'tests\corrupt.save')
Copy-Item '.\tests\invalid_event.save' (Join-Path $sandbox 'tests\invalid_event.save')
$output = Join-Path $sandbox 'output.txt'
try {
    foreach ($file in Get-ChildItem '.\tests\T*.in' | Sort-Object Name) {
        $count++
        $expectedPath = [IO.Path]::ChangeExtension($file.FullName, '.out')
        $cmdLine = 'cd /d "' + $sandbox + '" && "' + $binary + '" < "' + $file.FullName + '" > "' + $output + '" 2>nul'
        cmd /d /c $cmdLine | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "Program failed during $($file.Name)" }
        $expected = [IO.File]::ReadAllText($expectedPath).Replace("`r`n", "`n").TrimEnd("`n")
        $actual = [IO.File]::ReadAllText($output).Replace("`r`n", "`n").TrimEnd("`n")
        if ($actual -ceq $expected) {
            Write-Host "PASS $($file.Name)" -ForegroundColor Green
            $passed++
        } else {
            Write-Host "FAIL $($file.Name)" -ForegroundColor Red
            Write-Host "Expected:`n$expected`nActual:`n$actual"
        }
    }
} finally {
    Remove-Item $sandbox -Recurse -Force -ErrorAction SilentlyContinue
}
Write-Host "Passed $passed/$count tests"
if ($passed -ne $count) { exit 1 }
