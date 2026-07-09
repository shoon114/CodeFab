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
    @{ Category = "if/else if/else(3단 체인, 여러 줄, if 분기 참)"; InputLines = @('var a = 9;', 'var b = 0;', 'if (a > 3)', 'print "x";', 'else if (b > 1)', 'print "y";', 'else', 'print "z";'); Expect = "x" }
    @{ Category = "if/else if/else(3단 체인, 여러 줄, else-if 분기 참)"; InputLines = @('var a = 1;', 'var b = 5;', 'if (a > 3)', 'print "x";', 'else if (b > 1)', 'print "y";', 'else', 'print "z";'); Expect = "y" }
    @{ Category = "if/else if/else(3단 체인, 여러 줄, else 분기 참)"; InputLines = @('var a = 1;', 'var b = 0;', 'if (a > 3)', 'print "x";', 'else if (b > 1)', 'print "y";', 'else', 'print "z";'); Expect = "z" }
    @{ Category = "for 반복문"; InputLines = @('for (var j = 0; j < 3; j = j + 1) { print j; }'); Expect = "012" }
    @{ Category = "for 반복문(단일 줄 body)"; InputLines = @('for (var j = 0; j < 3; j = j + 1) print j;'); Expect = "012" }
    @{ Category = "함수 선언과 호출"; InputLines = @('func add(a, b) { return a + b; }', 'print add(2, 3);'); Expect = "5" }
    @{ Category = "함수 호출(return 없이 종료)"; InputLines = @('func noop() { }', 'print noop();'); Expect = "null" }
    @{ Category = "함수 호출(재귀, 팩토리얼)"; InputLines = @('func fact(n) { if (n <= 1) { return 1; } return n * fact(n - 1); }', 'print fact(5);'); Expect = "120" }
    @{ Category = "함수 호출(전역 변수 접근 가능)"; InputLines = @('var g = 10;', 'func showG() { return g; }', 'print showG();'); Expect = "10" }
    @{ Category = "함수 호출(for/if 내부에서 return으로 조기 종료)"; InputLines = @('func firstEvenFrom(start, n) { for (var i = start; i <= n; i = i + 1) { if (i % 2 == 0) { return i; } } return -1; }', 'print firstEvenFrom(3, 10);'); Expect = "4" }
    @{ Category = "함수 호출(내부 지역 변수가 바깥에 영향 없음)"; InputLines = @('var x = 1;', 'func setX() { var x = 99; return x; }', 'print setX();', 'print x;'); Expect = "99`n1" }
    @{ Category = "배열 생성/쓰기/읽기"; InputLines = @('var arr = Array(3);', 'arr[0] = 10;', 'print arr[0];'); Expect = "10" }
    @{ Category = "배열 기본값(null)"; InputLines = @('var arr = Array(3);', 'print arr[1];'); Expect = "null" }
    @{ Category = "배열(for 반복문으로 채우기)"; InputLines = @('var arr = Array(3);', 'for (var i = 0; i < 3; i = i + 1) { arr[i] = i * i; }', 'print arr[2];'); Expect = "4" }

    # 아래부터는 "정확한 출력값"이 아니라 "에러가 발생하는지"만 확인하는 케이스다
    # (ExpectError = $true). Expect 필드는 쓰지 않는다.
    @{ Category = "런타임 오류: 배열 인덱스 범위 초과"; InputLines = @('var arr = Array(3);', 'print arr[5];'); ExpectError = $true }
    @{ Category = "런타임 오류: 배열 음수 인덱스"; InputLines = @('var arr = Array(3);', 'print arr[-1];'); ExpectError = $true }
    @{ Category = "정적 오류: 함수 호출 인자 개수 불일치"; InputLines = @('func add(a, b) { return a + b; }', 'print add(1);'); ExpectError = $true }
    @{ Category = "정적 오류: 정의되지 않은 함수 호출"; InputLines = @('print notAFunc(1);'); ExpectError = $true }
    @{ Category = "런타임 오류: 함수는 호출자의 로컬 스코프에 접근 불가"; InputLines = @('func tryAccess() { return localOnly; }', '{ var localOnly = 5; print tryAccess(); }'); ExpectError = $true }
    @{ Category = "런타임 오류: for 단일 줄 body에서 선언된 변수는 바깥에서 참조 불가"; InputLines = @('for (var i = 0; i < 3; i = i + 1) var x = i;', 'print x;'); ExpectError = $true }
    @{ Category = "런타임 오류: for init에서 선언된 변수는 바깥에서 참조 불가"; InputLines = @('for (var a = 0; a < 3; a = a + 1) { print a; }', 'print a;'); ExpectError = $true }
    @{ Category = "구문 오류: 세미콜론 누락"; InputLines = @('print 1 + 2'); ExpectError = $true }
    @{ Category = "구문 오류: 닫는 괄호 누락"; InputLines = @('print (1 + 2;'); ExpectError = $true }
    @{ Category = "런타임 오류: 잘못된 할당 대상"; InputLines = @('var a = 1;', 'var b = 2;', 'a + b = 3;'); ExpectError = $true }
    @{ Category = "구문 오류: 표현식 자리에 엉뚱한 토큰"; InputLines = @('print * 5;'); ExpectError = $true }
    @{ Category = "정적 오류: 초기화식에서 자기 참조"; InputLines = @('{ var a = a; }'); ExpectError = $true }
    @{ Category = "정적 오류: 같은 스코프 중복 선언"; InputLines = @('{ var a = "hi"; var a = 3; }'); ExpectError = $true }
    @{ Category = "런타임 오류: 정의되지 않은 변수 참조"; InputLines = @('print notDefined;'); ExpectError = $true }
    @{ Category = "런타임 오류: + 연산자에 숫자/문자열 혼용"; InputLines = @('print 1 + "HI";'); ExpectError = $true }
    @{ Category = "런타임 오류: 숫자가 아닌 값에 단항 마이너스"; InputLines = @('print -"FabCoding";'); ExpectError = $true }
)

# Piping strings to a native exe (either via the PowerShell pipeline or via
# a .NET Process's StandardInput StreamWriter) reliably prefixes a UTF-8 BOM
# byte sequence onto the first token CodeFab reads in this environment,
# regardless of how the writer's encoding is configured. Native file
# redirection via cmd.exe (the same mechanism a real console uses) does not
# have this problem, so write the input to a temp file and redirect from it.
# stdout/stderr는 따로 캡처한다 -- ExpectError 케이스는 stderr(에러 메시지)가
# 비어있지 않은지만 확인하고, 그 외 케이스는 stdout만 기대값과 비교한다.
#
# PromptMode가 매 줄 입력 전에 ">>> "/"..> " 프롬프트를 stderr로 출력하므로
# (stdout으로 보내면 run.ps1의 stdout 비교가 깨지기 때문에 의도적으로 stderr로
# 분리되어 있음), 실제 에러 메시지 유무를 판단하려면 이 프롬프트 잡음을 먼저
# 제거해야 한다. 그러지 않으면 에러가 전혀 없는 정상 실행도 stderr가 항상
# 비어있지 않게 되어 ExpectError 판정이 무의미해진다.
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
        if ($stderr) {
            $stderr = ($stderr -replace '>>> ', '') -replace '\.\.> ', ''
            $stderr = $stderr.Trim()
        }
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
