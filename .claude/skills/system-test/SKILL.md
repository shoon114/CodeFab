---
name: system-test
description: CodeFab 콘솔 인터프리터의 Release 빌드를 대상으로, 지금까지 사람이 검증한 system test 케이스(산술 연산자 우선순위, 비교 연산자, 문자열 연결, 숫자/불리언 출력 포맷, 변수 선언·재할당 등)를 한 번의 빌드로 일괄 실행하고 기대 출력과 비교한다. 사용자가 "system test 해줘", "system test 돌려줘", "콘솔 입력 검증해줘" 등으로 요청할 때 사용한다.
---

# CodeFab System Test

CLAUDE.md 5절("system test 단계")에서 정의한 방식대로, 실제 콘솔 입력에 대한
CodeFab의 실행 결과를 검증하는 스킬이다. 지금까지 세션에서 사람이 하나씩
확인했던 케이스들을 `.claude/skills/system-test/run.ps1`에 누적해뒀고, 이
스킬은 그 케이스들을 **Release 빌드 1회 + 실행 N회**로 재현한다(케이스마다
다시 빌드하지 않는다).

## 실행 방법

PowerShell 도구로 아래 스크립트를 실행한다:

```
powershell -ExecutionPolicy Bypass -File .claude/skills/system-test/run.ps1
```

스크립트가 하는 일:
1. MSBuild로 `CodeFab.slnx`를 `Release|x64` 구성으로 1회 빌드한다.
2. 스크립트 안의 `$cases` 배열에 정의된 각 케이스를 이미 빌드된
   `x64/Release/CodeFab.exe`에 콘솔 입력(줄 단위)으로 그대로 넣어 실행하고
   출력을 캡처한다. `InputLines`가 여러 줄이면 실제 콘솔에서 한 줄씩 Enter를
   치는 것과 동일하게 여러 줄을 순서대로 입력한다.
3. 캡처된 출력에서 빈 줄(변수 선언처럼 print가 없는 줄이 만드는 개행)을
   제거한 뒤 `Expect`와 비교해 PASS/FAIL을 표로 출력한다.
4. 실패 개수를 프로세스 종료 코드로 반환한다(0이면 전부 PASS).

이미 최신 Release 빌드가 있고 다시 빌드할 필요가 없다면 `-SkipBuild` 옵션을
붙여서 실행 단계만 반복할 수 있다:

```
powershell -ExecutionPolicy Bypass -File .claude/skills/system-test/run.ps1 -SkipBuild
```

## 결과 보고 방법

- 스크립트가 출력한 표를 그대로 사용자에게 보여준다.
- FAIL이 있으면 해당 케이스의 Category/Input/Expect/Actual을 짚어서 어떤
  입력이 왜 다르게 나왔는지 설명한다.
- 전부 PASS면 몇 개 케이스가 통과했는지만 간단히 요약한다.

## 새 케이스 추가하기

사용자가 새로운 콘솔 입력/기대 출력 케이스를 알려주면, `run.ps1`의 `$cases`
배열 끝에 같은 해시테이블 형식으로 추가한다:

```powershell
@{ Category = "<분류명>"; InputLines = @('<한 줄씩 입력할 코드>', ...); Expect = "<기대 출력(빈 줄 제외, 여러 줄이면 개행으로 구분)>" }
```

- 한 줄짜리 입력이면 `InputLines`에 원소 하나만 넣는다.
- 변수 선언처럼 여러 줄에 걸친 시나리오는 실제 사용자가 콘솔에 입력하는
  순서 그대로 배열 원소를 나열한다.
- `Expect`는 print 등으로 실제 출력되는 값만 적는다(변수 선언 등 출력이
  없는 줄이 만드는 빈 줄은 스크립트가 자동으로 무시하고 비교한다).

케이스를 추가/수정한 뒤에는 커밋할지 사용자에게 확인한다 — 이 스킬 파일과
케이스 목록은 프로젝트에 커밋되어 다른 개발자도 동일한 회귀 세트를 쓰게
되므로, 회귀 케이스 추가 자체도 리뷰 대상으로 취급한다.

## 현재 포함된 케이스 요약

| 분류 | 입력 | 기대값 |
|---|---|---|
| 산술 연산자 우선순위 | `print 1 + 2 * 3;` | `7` |
| 산술 연산자 우선순위 | `print (1 + 2) * 3;` | `9` |
| 산술 연산자 우선순위 | `print 10 - 4 - 3;` | `3` |
| 산술 연산자 우선순위 | `print 8 / 2 / 2;` | `2` |
| 산술 연산자 우선순위 | `print -3 + 2;` | `-1` |
| 비교 연산자 | `print 1 < 2;` | `true` |
| 비교 연산자 | `print 3 > 5;` | `false` |
| 문자열 연결 | `print "Hello, " + "CodeFab!";` | `Hello, CodeFab!` |
| 숫자 출력 포맷 | `print 5;` | `5` |
| 숫자 출력 포맷 | `print 5.0;` | `5` |
| 숫자 출력 포맷 | `print 3.14;` | `3.14` |
| boolean 출력 | `print true;` | `true` |
| boolean 출력 | `print false;` | `false` |
| 변수 선언과 할당 | `var a = 10;` → `var b = 20;` → `print a + b;` | `30` |
| 변수 재할당 | `var a = 10;` → `var b = 20;` → `a = a + 5;` → `print a;` | `15` |
| 블록 스코프(여러 줄) | `var x = "global";` → `{` → `  var x = "inner";` → `  print x;` → `}` → `print x;` | `inner` (첫 print), `global` (두 번째 print) |
| 블록 스코프(여러 줄) | `var count = 0;` → `{` → `  count = count + 1;` → `}` → `print count;` | `1` (재선언이 아니라 바깥 변수를 수정) |
