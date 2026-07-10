# UnitTC(Debug exe, RUN_ALL_TESTS) + SystemTC(Release exe, run.ps1의 $cases)를
# 모두 실행하면서 OpenCppCoverage로 커버리지를 누적 수집하고, 최종적으로
# cobertura XML 하나로 병합해서 내보내는 스크립트.
#
# run.ps1의 $cases 배열은 그대로 재사용한다(정의를 복제하지 않기 위해
# run.ps1 텍스트에서 배열 리터럴만 추출해서 그 자리에서 평가한다).

param(
    [string]$OutXml = "$PSScriptRoot\..\..\..\docs\test_coverage.xml"
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$occ = "C:\Program Files\OpenCppCoverage\OpenCppCoverage.exe"
$debugExe = Join-Path $repoRoot "x64\Debug\CodeFab.exe"
$releaseExe = Join-Path $repoRoot "x64\Release\CodeFab.exe"
$wrapper = Join-Path $PSScriptRoot "redirect_run.cmd"
$workDir = Join-Path $env:TEMP "claude\coverage"
$sysDir = Join-Path $workDir "sys"

New-Item -ItemType Directory -Force -Path $sysDir | Out-Null

# ---- run.ps1에서 $cases 배열 리터럴만 추출 ----
$runPs1 = Get-Content (Join-Path $PSScriptRoot "run.ps1") -Raw
if ($runPs1 -notmatch '(?s)\$cases = @\(.*?\r?\n\)') {
    throw "run.ps1에서 `$cases 배열을 찾지 못했습니다."
}
Invoke-Expression $matches[0]
Write-Host ("run.ps1에서 SystemTC {0}개를 로드했습니다." -f $cases.Count) -ForegroundColor Cyan

$commonArgs = @(
    "--sources", "$repoRoot\*",
    "--excluded_sources", "$repoRoot\x64\*",
    "--excluded_sources", "*\Test*.cpp",
    "--excluded_sources", "*\MockStatementParser.h",
    "--excluded_sources", "*\TestTokenHelpers.h"
)

# ---- 1) UnitTC: Debug exe (RUN_ALL_TESTS) ----
$unitCov = Join-Path $workDir "unit.cov"
Write-Host "UnitTC 커버리지 수집 중 (Debug exe, gtest/gmock 전체 실행)..." -ForegroundColor Cyan
& $occ @commonArgs --export_type "binary:$unitCov" -- $debugExe | Out-Null
if (-not (Test-Path $unitCov)) { throw "UnitTC 커버리지 수집 실패" }

# ---- 2) SystemTC: Release exe, 케이스별로 별도 프로세스 실행 ----
$covFiles = @($unitCov)
$i = 0
foreach ($case in $cases) {
    $i++
    $tmpFile = [System.IO.Path]::GetTempFileName()
    $errFile = [System.IO.Path]::GetTempFileName()
    $covFile = Join-Path $sysDir ("case_{0:D3}.cov" -f $i)
    try {
        $content = ($case.InputLines -join "`n") + "`n"
        [System.IO.File]::WriteAllText($tmpFile, $content, (New-Object System.Text.UTF8Encoding($false)))

        & $occ @commonArgs --cover_children --export_type "binary:$covFile" `
            -- cmd /c $wrapper $releaseExe $tmpFile "$errFile.out" $errFile | Out-Null

        if (Test-Path $covFile) {
            $covFiles += $covFile
        } else {
            Write-Warning ("케이스 {0} ({1}) 커버리지 파일이 생성되지 않았습니다." -f $i, $case.Category)
        }
    }
    finally {
        Remove-Item $tmpFile, $errFile, "$errFile.out" -ErrorAction SilentlyContinue
    }
    if ($i % 10 -eq 0) { Write-Host ("  SystemTC {0}/{1} 완료" -f $i, $cases.Count) }
}
Write-Host ("SystemTC {0}개 전체 커버리지 수집 완료" -f $cases.Count) -ForegroundColor Cyan

# ---- 3) 전체 병합 -> 최종 cobertura XML ----
Write-Host "전체 결과 병합 중..." -ForegroundColor Cyan
$inputArgs = @()
foreach ($f in $covFiles) { $inputArgs += "--input_coverage"; $inputArgs += $f }

New-Item -ItemType Directory -Force -Path (Split-Path $OutXml) | Out-Null

& $occ @commonArgs @inputArgs --export_type "cobertura:$OutXml" -- cmd /c ver | Out-Null

if (-not (Test-Path $OutXml)) { throw "최종 커버리지 XML 생성 실패" }
Write-Host ("최종 커버리지 XML 저장됨: {0}" -f $OutXml) -ForegroundColor Green
