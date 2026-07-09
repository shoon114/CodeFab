# CodeFab system test runner
#
# Release|x64 빌드를 한 번만 수행한 뒤, 이미 빌드된 CodeFab.exe에 대해
# 아래 $cases에 정의된 콘솔 입력들을 각각 실행하고 기대 출력과 비교한다.
# 새 케이스는 $cases 배열에 같은 형식(Category/InputLines/Expect)으로
# 추가하면 된다.

param(
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$sln = Join-Path $repoRoot "CodeFab.slnx"
$exePath = Join-Path $repoRoot "x64\Release\CodeFab.exe"

if (-not $SkipBuild) {
    Write-Host "Building Release|x64 ..." -ForegroundColor Cyan

    $msbuild = Get-ChildItem "C:\Program Files\Microsoft Visual Studio" -Recurse -Filter MSBuild.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $msbuild) {
        throw "MSBuild.exe not found under 'C:\Program Files\Microsoft Visual Studio'."
    }

    & $msbuild.FullName $sln /p:Configuration=Release /p:Platform=x64 /m /v:minimal
    if ($LASTEXITCODE -ne 0) {
        throw "Release build failed (exit code $LASTEXITCODE)."
    }
}

if (-not (Test-Path $exePath)) {
    throw "Built exe not found at $exePath. Run without -SkipBuild first."
}

# 콘솔에 실제로 입력하는 줄들을 InputLines에 순서대로 나열한다(각 원소가 Enter 한 번).
# Expect는 빈 줄을 제외한 실제 출력 전체(줄바꿈은 " / "로 표시하지 않고 그대로 개행 유지)와 비교한다.
$cases = @(
    @{ Category = "산술 연산자 우선순위"; InputLines = @('print 1 + 2 * 3;');       Expect = "7" }
    @{ Category = "산술 연산자 우선순위"; InputLines = @('print (1 + 2) * 3;');     Expect = "9" }
    @{ Category = "산술 연산자 우선순위"; InputLines = @('print 10 - 4 - 3;');      Expect = "3" }
    @{ Category = "산술 연산자 우선순위"; InputLines = @('print 8 / 2 / 2;');       Expect = "2" }
    @{ Category = "산술 연산자 우선순위"; InputLines = @('print -3 + 2;');          Expect = "-1" }
    @{ Category = "비교 연산자";         InputLines = @('print 1 < 2;');           Expect = "true" }
    @{ Category = "비교 연산자";         InputLines = @('print 3 > 5;');           Expect = "false" }
    @{ Category = "문자열 연결";         InputLines = @('print "Hello, " + "CodeFab!";'); Expect = "Hello, CodeFab!" }
    @{ Category = "숫자 출력 포맷";      InputLines = @('print 5;');               Expect = "5" }
    @{ Category = "숫자 출력 포맷";      InputLines = @('print 5.0;');             Expect = "5" }
    @{ Category = "숫자 출력 포맷";      InputLines = @('print 3.14;');            Expect = "3.14" }
    @{ Category = "boolean 출력";       InputLines = @('print true;');            Expect = "true" }
    @{ Category = "boolean 출력";       InputLines = @('print false;');           Expect = "false" }
    @{ Category = "변수 선언과 할당";    InputLines = @('var a = 10;', 'var b = 20;', 'print a + b;'); Expect = "30" }
    @{ Category = "변수 재할당";         InputLines = @('var a = 10;', 'var b = 20;', 'a = a + 5;', 'print a;'); Expect = "15" }
    @{ Category = "블록 스코프(여러 줄)"; InputLines = @('var x = "global";', '{', '  var x = "inner";', '  print x;', '}', 'print x;'); Expect = "inner`nglobal" }
    @{ Category = "블록 스코프(여러 줄)"; InputLines = @('var count = 0;', '{', '  count = count + 1;', '}', 'print count;'); Expect = "1" }
    @{ Category = "블록 스코프(여러 줄)"; InputLines = @('var outer = "A";', '{', '  var inner = "B";', '  {', '    print outer + inner;', '  }', '}'); Expect = "AB" }
    @{ Category = "if/else"; InputLines = @('if (true) { print "bbq"; }'); Expect = "bbq" }
    @{ Category = "if/else"; InputLines = @('if (false) { print "no"; } else { print "kfc"; }'); Expect = "kfc" }
    @{ Category = "if/else(여러 줄, 중첩)"; InputLines = @('if (true)', '{', '  if (false) { print "kfc"; }', '  else { print "bbq"; }', '}'); Expect = "bbq" }
    @{ Category = "if/else if(앞에 완결된 블록이 있고 여러 줄)"; InputLines = @('var a = 5;', 'var b = 2;', 'if (a > 3) { print "x"; } else if (b > 1)', '{ print "y"; }'); Expect = "x" }
    @{ Category = "if/else if(앞에 완결된 블록이 있고 여러 줄, else-if 분기)"; InputLines = @('var a = 1;', 'var b = 2;', 'if (a > 3) { print "x"; } else if (b > 1)', '{ print "y"; }'); Expect = "y" }
    @{ Category = "if/else if(if body가 별도 줄, 둘 다 거짓)"; InputLines = @('var a = 1;', 'var b = 0;', 'if (a > 3)', '{ print "x"; }', 'else if (b > 1)', '{ print "y"; }'); Expect = "" }
    @{ Category = "if/else if(if body가 별도 줄, else-if 분기 참)"; InputLines = @('var a = 1;', 'var b = 5;', 'if (a > 3)', '{ print "x"; }', 'else if (b > 1)', '{ print "y"; }'); Expect = "y" }
    @{ Category = "if/else if(if body가 별도 줄, if 분기 참)"; InputLines = @('var a = 9;', 'var b = 5;', 'if (a > 3)', '{ print "x"; }', 'else if (b > 1)', '{ print "y"; }'); Expect = "x" }
    @{ Category = "if/else if(body가 '{}' 없는 단일 문장, 둘 다 거짓)"; InputLines = @('var a = 1;', 'var b = 0;', 'if (a > 3)', 'print "x";', 'else if (b > 1)', 'print "y";'); Expect = "" }
    @{ Category = "if/else if(body가 '{}' 없는 단일 문장, else-if 분기 참)"; InputLines = @('var a = 1;', 'var b = 5;', 'if (a > 3)', 'print "x";', 'else if (b > 1)', 'print "y";'); Expect = "y" }
    @{ Category = "if/else if(body가 '{}' 없는 단일 문장, if 분기 참)"; InputLines = @('var a = 9;', 'var b = 5;', 'if (a > 3)', 'print "x";', 'else if (b > 1)', 'print "y";'); Expect = "x" }
    @{ Category = "if(else 없음, body가 '{}' 없는 단일 문장, 뒤에 무관한 문장)"; InputLines = @('var a = 1;', 'if (a > 3)', 'print "x";', 'var c = 1;', 'print c;'); Expect = "1" }
    @{ Category = "for 반복문"; InputLines = @('for (var j = 0; j < 3; j = j + 1) { print j; }'); Expect = "012" }

    # 아래부터는 "정확한 출력값"이 아니라 "에러가 발생하는지"만 확인하는 케이스다
    # (ExpectError = $true). Expect 필드는 쓰지 않는다.
    @{ Category = "구문 오류: 세미콜론 누락"; InputLines = @('print 1 + 2'); ExpectError = $true }
    @{ Category = "구문 오류: 닫는 괄호 누락"; InputLines = @('print (1 + 2;'); ExpectError = $true }
    @{ Category = "런타임 오류: 잘못된 할당 대상"; InputLines = @('var a = 1;', 'var b = 2;', 'a + b = 3;'); ExpectError = $true }
    @{ Category = "구문 오류: 표현식 자리에 엉뚱한 토큰"; InputLines = @('print * 5;'); ExpectError = $true }
)

# Piping strings to a native exe (either via the PowerShell pipeline or via
# a .NET Process's StandardInput StreamWriter) reliably prefixes a UTF-8 BOM
# byte sequence onto the first token CodeFab reads in this environment,
# regardless of how the writer's encoding is configured. Native file
# redirection via cmd.exe (the same mechanism a real console uses) does not
# have this problem, so write the input to a temp file and redirect from it.
# stdout/stderr는 따로 캡처한다 -- ExpectError 케이스는 stderr(에러 메시지)가
# 비어있지 않은지만 확인하고, 그 외 케이스는 stdout만 기대값과 비교한다.
function Invoke-CodeFab {
    param(
        [string]$ExePath,
        [string[]]$Lines
    )

    $tmpFile = [System.IO.Path]::GetTempFileName()
    $errFile = [System.IO.Path]::GetTempFileName()
    try {
        $content = ($Lines -join "`n") + "`n"
        [System.IO.File]::WriteAllText($tmpFile, $content, (New-Object System.Text.UTF8Encoding($false)))
        $stdout = cmd /c "`"$ExePath`" < `"$tmpFile`" 2>`"$errFile`""
        $stderr = Get-Content -Raw -ErrorAction SilentlyContinue -Path $errFile
        return [pscustomobject]@{
            Stdout = ($stdout -join "`n")
            Stderr = if ($stderr) { $stderr } else { "" }
        }
    }
    finally {
        Remove-Item $tmpFile -ErrorAction SilentlyContinue
        Remove-Item $errFile -ErrorAction SilentlyContinue
    }
}

$results = @()
foreach ($case in $cases) {
    $output = Invoke-CodeFab -ExePath $exePath -Lines $case.InputLines

    if ($case.ExpectError) {
        $pass = -not [string]::IsNullOrWhiteSpace($output.Stderr)
        $expectDisplay = "(에러 발생)"
        $actual = if ($pass) { $output.Stderr.Trim() } else { "(에러 없음)" }
    } else {
        $actualLines = $output.Stdout -split "`r?`n" | Where-Object { $_.Trim() -ne "" }
        $actual = ($actualLines -join "`n").Trim()
        $pass = ($actual -eq $case.Expect)
        $expectDisplay = $case.Expect
    }

    $results += [pscustomobject]@{
        Category = $case.Category
        Input    = ($case.InputLines -join " ")
        Expect   = $expectDisplay
        Actual   = $actual
        Result   = if ($pass) { "PASS" } else { "FAIL" }
    }
}

$results | Format-Table -AutoSize -Wrap | Out-String -Width 200 | Write-Host

$failCount = ($results | Where-Object { $_.Result -eq "FAIL" }).Count
Write-Host ""
if ($failCount -eq 0) {
    Write-Host ("전체 {0}개 케이스 모두 PASS" -f $results.Count) -ForegroundColor Green
} else {
    Write-Host ("{0}개 중 {1}개 FAIL" -f $results.Count, $failCount) -ForegroundColor Red
}

exit $failCount
