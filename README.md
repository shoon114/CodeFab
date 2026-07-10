# CodeFab

간단한 문법으로 변수, 함수, 배열, 클래스, `for`/`if` 제어문을 지원하는 C++ 기반 인터프리터입니다.

## 목차

- [빌드 및 실행](#빌드-및-실행)
- [실행 모드](#실행-모드)
- [구현된 기능](#구현된-기능)
- [언어 문법 및 구문 규칙](#언어-문법-및-구문-규칙)
- [정적 검사 및 런타임 오류 처리](#정적-검사-및-런타임-오류-처리)
- [예제](#예제)
- [테스트](#테스트)
- [프로젝트 구조](#프로젝트-구조)
- [문서 구조](#문서-구조)
- [진행 상황](#진행-상황)

## 빌드 및 실행

Visual Studio(MSBuild, x64)로 빌드합니다.

```powershell
MSBuild.exe CodeFab.vcxproj /p:Configuration=Release /p:Platform=x64
```

빌드 결과물은 `x64\Release\CodeFab.exe` 에 생성됩니다.

> Debug 구성으로 빌드하면 실행 파일이 인터프리터 대신 GoogleTest 테스트 스위트를 실행합니다 (아래 [테스트](#테스트) 참고).

## 실행 모드

| 명령 | 설명 |
|---|---|
| `CodeFab.exe` | 인자 없이 실행하면 REPL(대화형 프롬프트) 모드로 진입합니다. |
| `CodeFab.exe run <file>` | 지정한 소스 파일을 파싱 → 정적 검사 → 실행까지 한 번에 수행합니다. |
| `CodeFab.exe debug <file>` | 디버그 모드 (현재 stub — 구현 예정). |

## 구현된 기능

| 기능 | 지원 여부 | 비고 |
|---|---|---|
| 변수 선언/대입 | ✅ | `var` |
| 산술/비교/논리 연산자 | ✅ | `+ - * /`, `< > <= >= == !=`, `&& \|\| !` |
| 제어문 | ✅ | `if`/`else`, `for` |
| 함수 선언/호출 | ✅ | `func`, `return` |
| 배열 | ✅ | `Array(size)`, `arr[index]` 읽기/쓰기 |
| 클래스 | ✅ | 필드, 메서드, `this`, `init` 생성자 |
| 클래스 상속 | ✅ | `class B : A { ... }`, `super.method()` |
| `instanceof` | ✅ | `value instanceof ClassName` |
| 출력 | ✅ | `print expr;` |
| 정적 바인딩 최적화 | ✅ | 변수 참조의 스코프 거리를 실행 전에 계산(`scopeDistance`), 런타임에 스코프를 매번 탐색하지 않음 |
| 상수 폴딩 최적화 | ✅ | 리터럴로만 구성된 산술/비교/논리식을 실행 전에 미리 계산 |
| `import` | ⚠️ | 파일을 토큰 단위로 splice하는 방식으로 동작. `alias`는 아직 네임스페이싱에 사용되지 않음(inert) |
| 디버그 모드(`debug <file>`) | ❌ | CLI 스켈레톤만 존재, 미구현(stub) |

## 언어 문법 및 구문 규칙

| 규칙 | 설명 |
|---|---|
| 문장 구분 | 모든 문장은 세미콜론(`;`)으로 종료합니다. |
| 파일 종료 | 코드의 끝은 개행 문자(`\n`)로 식별합니다. |
| 변수 선언 | `var a = 0;` |
| 제어문 | `if`/`else`, `for (var i = 0; i < N; i = i + 1) { ... }` |
| 함수 | `func add(a, b) { return a + b; }` |
| 배열 | `Array(size)`, `arr[index]` |
| 클래스 | `class Robot { init(name) { this.name = name; } }`, 상속(`:`), `super`, `instanceof` |
| 출력 | `print expr;` |

## 정적 검사 및 런타임 오류 처리

실행 전 `CheckerUnit`이 AST만으로 검증 가능한 것들을 미리 잡아내고, `ExecutorUnit`이 실제 실행 시점에만 알 수 있는 오류를 처리합니다.

**실행 전(정적) 검사 — `CheckerUnit`**

| 검사 항목 | 예시 |
|---|---|
| 변수/함수/클래스 중복 선언 | `var a; var a;` |
| 자기 초기화식에서 자신 참조 | `var a = a;` |
| 정의되지 않은 함수/클래스 호출 | `foo();` (foo 미선언) |
| 반복문 내부에서 `import` 사용 | `for (...) { import "x" alias y; }` |
| 클래스 외부에서 `this`/`super` 사용 | 클래스 메서드 밖에서 `this.x` |
| 부모 없는 클래스에서 `super` 사용 | `class A { m() { super.m(); } }` |
| `init`(생성자) 내부에서 `return` | `init() { return 1; }` |
| 함수 외부의 `return` | 최상위 레벨의 `return;` |
| 리터럴로 판단 가능한 배열 크기/인덱스 오류 | `Array("x")`, `arr["x"]`, 리터럴 범위 초과 인덱스 |

**실행 시점(런타임) 오류 — `ExecutorUnit`**

| 오류 | 예시 |
|---|---|
| 0으로 나누기 | `1 / 0;` |
| 선언되지 않은 변수 참조 | 정적 검사를 통과했지만 실제 실행 경로에서 미선언 변수에 도달 |
| 숫자가 아닌 값에 산술 연산 | `"a" + true;` 등 피연산자 타입 오류 |
| 배열이 아닌 값에 인덱스 접근 | `var x = 1; x[0];` |
| 배열 인덱스가 범위를 벗어남 | 변수로 계산된 인덱스가 실행 시점에 범위를 벗어남 |
| 대입 대상이 아닌 식에 대입 | `1 = 2;` |
| 인스턴스가 아닌 값의 필드/메서드 접근 | 클래스 인스턴스가 아닌 값에 `.field`/`.method()` |
| 존재하지 않는 필드/메서드 접근 | 클래스에 선언되지 않은 `.field`/`.method()` 호출 |

> `CheckerUnit`은 리터럴/구조적으로 확정 가능한 경우만 검사합니다(값 흐름 추적은 하지 않음). 그래서 변수를 통한 동적인 값에 의한 오류(예: 변수로 계산된 배열 인덱스 초과, 변수가 실제로 어떤 타입인지)는 실행 시점에 `ExecutorUnit`이 잡아냅니다.

## 예제

```
var a = 0;
for (var i = 0; i < 5; i = i + 1) {
    a = a + i;
}
print a;
```

```
class Animal {
    init(name) {
        this.name = name;
    }
    speak() {
        print this.name;
    }
}

class Dog : Animal {
    speak() {
        super.speak();
        print "Woof";
    }
}

var d = Dog("Rex");
d.speak();
```

## 테스트

Debug 빌드는 GoogleTest 기반의 유닛 테스트 전체를 실행 파일 자체로 포함합니다.

```powershell
MSBuild.exe CodeFab.vcxproj /p:Configuration=Debug /p:Platform=x64
x64\Debug\CodeFab.exe
```

## 프로젝트 구조

| 모듈 | 역할 |
|---|---|
| `Tokenizer` | 소스 코드를 토큰 스트림으로 변환 |
| `AssemblerUnit` | 토큰을 파싱해 AST(`SyntaxNode` 트리)로 구성 |
| `CheckerUnit` | 실행 전 정적 검사(타입/스코프/중복 선언 등) 및 최적화(정적 바인딩, 상수 폴딩) 수행 |
| `ExecutorUnit` | AST를 순회하며 실제로 실행 |

자세한 아키텍처/설계 결정 사항은 [`CLAUDE.md`](CLAUDE.md), [`docs/`](docs) 디렉터리를 참고하세요.

## 문서 구조

설계/발표 자료는 전부 [`docs/`](docs) 디렉터리에 있습니다.

| 문서 | 내용 |
|---|---|
| [`docs/CodeFab_Design.md`](docs/CodeFab_Design.md) | 전체 설계 문서. 파이프라인, Parser의 Strategy+Registry 구조, `SyntaxNode`의 Visitor 패턴, `ShellMode`, 현재 미구현 항목(Import 실행부/DebugMode)까지 현재 구현 기준으로 정리 |
| [`docs/CodeFab_Design_Presentation.md`](docs/CodeFab_Design_Presentation.md) / [`.html`](docs/CodeFab_Design_Presentation.html) / [`.pdf`](docs/CodeFab_Design_Presentation.pdf) | 위 설계를 발표용으로 재구성한 자료(패턴별 UML 다이어그램 포함). md가 원본, html/pdf는 렌더링 산출물 |
| [`docs/CodeFab_Visitor_Pattern.md`](docs/CodeFab_Visitor_Pattern.md) | `SyntaxNode`/`NodeVisitor` 더블 디스패치 구조에 대한 상세 설명 |
| [`docs/feature_Import_design.md`](docs/feature_Import_design.md) | Import 기능의 에러 처리 책임 분담 설계. 최초 설계와 실제 구현(Tokenizer 스플라이스 방식)의 차이, 아직 미결정인 오픈 이슈(alias 네임스페이싱 등)를 기록 |
| [`docs/feature_class_design.md`](docs/feature_class_design.md) | Class 기능(상속, `this`/`super`, 생성자) 설계 문서 |
| [`docs/assets/*.svg`](docs/assets) | 발표 자료에 쓰이는 패턴별(Pipeline/Registry+Factory/Strategy/Composite/Visitor/Singleton) UML 다이어그램 |

> `src/skelton.cpp`와 `docs/CodeFab_Design.md`의 구버전 서술 일부는 outdated 배경이 있었으나(자세한 내용은 [`CLAUDE.md`](CLAUDE.md) 1절), `CodeFab_Design.md`는 2026-07-10에 현재 구현 기준으로 전면 개정되었습니다. `CLAUDE.md`가 아키텍처 관련 최우선 참고 문서입니다.

## 진행 상황

**구현 완료 (2026-07-10 기준)**: 변수/제어문/함수/배열/클래스(상속 포함)/`instanceof`/정적 바인딩·상수 폴딩 최적화 — 위 [구현된 기능](#구현된-기능) 표 참고. Import는 파서(`ImportStatementParser`)와 `CheckerUnit`의 정적 검사(alias 충돌/중복 import/반복문 내부 금지)까지는 실제로 동작합니다.

**진행 중 / 미구현**:

| 항목 | 상태 | 비고 |
|---|---|---|
| Import 실행부(`ModuleLoader`, alias 네임스페이스) | 미구현 | `ExecutorUnit`에 `Visit(const ImportStmtNode&)`가 없어 alias가 아직 아무 역할도 하지 않음. 실제 코드 유입은 `Tokenizer::ResolveImports`의 텍스트 splice에만 의존. 자세한 내용은 [`docs/feature_Import_design.md`](docs/feature_Import_design.md) |
| `DebugMode`(`debug <file>`) | 스텁 | `step/next/break/breakpoints/watch/inspect` 등 전부 미구현. 진입만 가능하고 안내 메시지만 출력 |

**테스트 전략 전환**: 각 모듈 API를 유닛 테스트로 검증하는 TDD 단계는 완료되었고, 이후 신규 기능/버그 수정은 system test(콘솔 입력 → 실행 결과 검증) 기준으로 진행합니다. 기존 유닛 테스트(TC)는 회귀 테스트로 계속 유지하며 커밋 전 항상 전체 스위트를 통과시킵니다. 자세한 내용은 [`CLAUDE.md`](CLAUDE.md) 5절 참고.
