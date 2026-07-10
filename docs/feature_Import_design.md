# Library Import 기능 — 에러 처리 책임 분담 정책

> **2026-07-10 최종 업데이트**: `alias` 네임스페이싱이 **실제로 구현 완료됐다**(`bd03407`/`eabc281`, #94/#96/#102). 6절에 "가정 시나리오"로 적어뒀던 `ImportEnd` 마커 기반 설계가 그대로 현실이 됐다 — 6절은 더 이상 가정이 아니라 실제 구현 설명이다. 1~5절(최초 설계)과 0절(Tokenizer 스플라이스 도입 경위)은 "왜 이런 구조가 됐는지"의 배경 기록으로 유지한다. 최신 구현 상태를 빠르게 확인하려면 6절부터 읽을 것.
>
> **2026-07-09 업데이트**: 아래 1~5절은 최초 설계(파일 로딩을 실행 시점 `ModuleLoader`가 담당)를 그대로 기록한 것이다. 그런데 실제로 병합된 PR #76(`feature/tokenizer_import_file`)은 **Tokenizer 단계에서 대상 파일을 재귀적으로 읽어 토큰 스트림에 그대로 이어붙이는(C의 `#include`와 유사한 텍스트 스플라이스) 방식**으로 구현되었다. 이는 아래 설계와 실행 시점/네임스페이싱 부분에서 실질적으로 다르다. 정확한 현황은 **0절**을 먼저 읽을 것 — 1~5절은 "최초 계획이 무엇이었는지"와 "CheckerUnit이 왜 그렇게 만들어졌는지"의 배경 기록으로 유지한다.

## 0. 현황 업데이트 — Tokenizer 스플라이스 방식 (PR #76 반영)

### 0.1 실제 구현된 동작

`Tokenizer::ResolveImports`가 토큰화 단계에서 다음을 수행한다:

1. 토큰 스트림에서 `KwImport String KwAlias Identifier Semicolon` 패턴을 찾는다.
2. `import` 뒤의 문자열이 가리키는 파일을 열어 재귀적으로 토큰화한다(그 파일 안에 또 `import`가 있으면 재귀 처리).
3. **`import ... ;` 문의 5개 토큰 자체는 지우지 않고 그대로 남긴 채**, 그 바로 뒤에 대상 파일의 토큰들을 이어붙인다.

즉 `import "math.cf" alias math;` 뒤에 오는 것은 `math.cf`의 내용이 **네임스페이스 없이 그냥 최상위 토큰으로 펼쳐진 것**이다 — `func add(a, b) {...}`가 스플라이스되면 파서 입장에서는 평범한 전역 함수 선언과 구분되지 않는다.

### 0.2 최초 설계와 달라진 점

| 항목 | 최초 설계(1~5절) | 실제 구현(PR #76) |
|---|---|---|
| 파일 존재 확인 시점 | 실행 시점, `ModuleLoader` | **토큰화 시점**, `Tokenizer::ReadFile` (파일 없으면 `"Cannot open import file: ..."`) |
| 순환 import 감지 시점 | 실행 시점, `ModuleLoader`가 "현재 로딩 중" 스택 유지 | **토큰화 시점**, `Tokenizer`가 `activeImports` 스택으로 감지(경로는 `std::filesystem::weakly_canonical`로 정규화해 비교) |
| `alias`의 역할 | export들을 감싼 `ModuleObject`를 가리키는 이름(`math.add(...)`처럼 멤버 접근) | **아무 역할 없음(inert)** — 토큰화 단계에서는 그냥 다음 식별자 토큰일 뿐, 네임스페이싱/리네이밍이 전혀 없음 |
| 새 파이프라인 단계 필요 여부 | 불필요("Parse와 Check 사이에 별도 단계 없이") | 여전히 불필요하지만, 대신 **Tokenizer 자체가 무거워짐**(파일 I/O + 재귀 파싱을 토큰화 단계에서 수행) |

코드 주석("ModuleLoader가 나중에 도입되면 순환 import 감지 책임은 그쪽으로 옮겨간다")을 보면 이 방식은 **의도된 임시 조치**로 보인다 — 다만 문서화된 계획이 없으면 다음 담당자가 "설계문서(1~5절)대로 되어 있다"고 오해할 수 있어 이 절을 추가했다.

### 0.3 CheckerUnit 구현에 대한 실질적 영향

`feature/CheckerImport`에서 이미 구현한 CheckerUnit 쪽(2~3.2절 내용)은 **코드 레벨 충돌은 없다** — PR #76은 Tokenizer만 건드렸다. **(2026-07-10 정정)** 이후 PR #86(`Feature/assembler unit/import`)에서 `ImportStatementParser`가 실제로 추가되어, `import "경로" alias 별칭;` 문이 이제 진짜로 `ImportStmtNode`로 파싱되고 `CheckerUnit::Visit(const ImportStmtNode&)`가 실행 경로에서 동작한다 — 더 이상 "죽은 코드"가 아니다. **(같은 날 추가 정정)** 그리고 PR #94/#96(`bd03407`)에서 `ExecutorUnit::Visit(const ImportStmtNode&)`가 추가되고, PR #102(`eabc281`)에서 `ImportEnd` 경계 마커 처리가 다듬어져 **`alias` 네임스페이스 바인딩(`ModuleObject`)까지 실제로 동작한다** — "import 문 자체는 파싱·검사된다"뿐 아니라 "alias가 실제로 뭔가를 가리킨다"도 이제 참이다. 자세한 최종 구조는 6절 참고. 아래 항목들은 이 정정을 반영해 다시 읽을 것.

- **"같은 scope 중복 import" 체크(`importScopeStack`)는 여전히, 오히려 더 필요하다.** Tokenizer는 같은 파일을 몇 번 import하든 그냥 여러 번 스플라이스한다(중복 방지 없음). 그리고 현재 `CheckerUnit::Visit(FuncDeclStmtNode&)`는 같은 스코프 내 함수 이름 중복을 검사하지 않으므로(`VarDeclareStatementNode`만 검사함), 같은 파일을 두 번 import하면 두 번째 스플라이스가 첫 번째 함수 선언을 아무 에러 없이 조용히 덮어쓸 수 있다. `importScopeStack` 기반 체크가 이 경우를 명확한 에러로 잡아주는 유일한 방어선이다.
- **`alias` 충돌 체크는 유지하되, "무엇을 보호하는지"가 아직 미정이다.** 지금은 alias를 "현재 스코프에 선언되는 이름"으로 취급해 변수 중복 체크와 동일하게 검사하지만, 실제로 alias가 무언가를 가리키는 네임스페이스 객체가 될지, 여전히 아무 의미 없는 토큰으로 남을지는 Parser/Checker/Executor 담당자가 추가로 결정해야 한다(아래 오픈 이슈 5번 참고).
- **파일없음/순환 import는 이제 CheckerUnit이 절대 볼 일이 없다** — Tokenizer가 그보다 먼저(Parse도 시작하기 전에) 예외를 던지고 끝나기 때문이다. 원래 설계(1~5절)에서도 CheckerUnit 담당이 아니었으니 실질적 차이는 없지만, "왜 CheckerUnit 테스트에 이 두 케이스가 없는지" 궁금할 사람을 위해 명시해둔다.

## 1. 배경

Library Import(`import "경로" alias 별칭;`) 기능을 도입하면서, 관련 에러들을 어느 모듈이 검출/보고할지에 대한 책임 경계를 정한다. 기준은 **SRP(단일 책임 원칙)**: 각 모듈이 "왜 바뀌는가(reason to change)"가 다르면 책임을 분리한다.

- `Parser`: 문법(토큰 시퀀스)이 유효한가만 본다. 파일/스코프/타입 등 의미는 모른다.
- `CheckerUnit`: 이미 파싱된 AST 구조만 보고 판단 가능한 정적 규칙을 검사한다. **파일시스템에 접근하지 않는다.**
- `Interpreter(ExecutorUnit)`: 실제 실행 시점에 값/자원을 다루며 발생하는 문제를 처리한다.
- `ModuleLoader`(신규): import 대상 파일을 읽고 재귀적으로 Tokenize/Parse하는 역할을 전담하는 런타임 헬퍼. `ExecutorUnit`이 `ImportStmtNode`를 실행할 때 호출한다.

## 2. 책임 분담 표

| 에러 케이스 | 담당 모듈 | 검출 시점 |
|---|---|---|
| import 문법 오류 | **Parser** (`ImportStmtParser`) | 파싱 시점 |
| import 대상 파일 없음 | **Interpreter** (`ExecutorUnit`, `ModuleLoader` 경유) | 실행 시점 |
| 같은 scope 내 중복 import | **CheckerUnit** | 정적 검사 시점 |
| 상위(조상) scope에서 이미 import된 파일 재-import | **CheckerUnit** (위와 동일 메커니즘) | 정적 검사 시점 |
| alias name 충돌 | **CheckerUnit** | 정적 검사 시점 |
| 반복문(`for`) 내부에서 import 문 사용 | **CheckerUnit** | 정적 검사 시점 |
| 순환 import (예: a.txt → b.txt → a.txt) | **ModuleLoader** | 실행 시점 (import 실행 중 파일 로딩 단계) |

## 3. 모듈별 상세

### 3.1 Parser (`ImportStmtParser`)
- 문법: `import <StringLiteral> alias <Identifier> ;`
- 이 형태에서 벗어나면(`import` 뒤에 문자열이 아님, `alias` 키워드 누락, 식별자 누락, `;` 누락 등) 즉시 `std::runtime_error`로 파싱 실패 처리.
- 파일 존재 여부, 순환 여부, 스코프 규칙은 전혀 모르며 알 필요도 없다.

### 3.2 CheckerUnit
기존에 이미 있는 "스코프 스택 기반 이름 충돌 검사"(변수 중복 선언), "구문 위치 제약 검사"(return은 함수 내부에서만)와 **동일한 패턴**으로 확장한다. 파일시스템에는 손대지 않는다.

- **같은 scope 내 중복 import / 상위 scope 재-import 금지**
  - `scopeStack`과 나란히 "스코프별로 이미 import한 파일 경로 집합" 스택을 신설.
  - `EnterScope`/`ExitScope`에 맞춰 push/pop.
  - `Visit(ImportStmtNode&)`에서 **현재 스코프부터 바깥으로(조상 스코프까지)** 훑어 같은 경로가 이미 있으면 에러.
  - 이 방식은 형제 스코프(`{import a}{import a}` → 정상)와 조상-자손 스코프(`{import a{import a}}` → 에러)를 스코프 스택 구조 자체가 자연스럽게 구분해준다. **"같은 scope 중복"과 "상위 scope 재-import 금지"는 별개 구현이 아니라 이 하나의 스택 탐색 로직으로 함께 처리된다.**

- **alias name 충돌**
  - alias를 현재 스코프의 변수명처럼 취급해 기존 `VarDeclareStatementNode` 중복 선언 검사 로직을 재사용.

- **반복문 내 import 사용 금지**
  - `functionDepth`(함수 내부 여부 추적)와 동일한 패턴으로 `loopDepth` 카운터 신설.
  - `Visit(const ForStmtNode&)`를 새로 추가해 body 진입/이탈 시 `loopDepth`를 증감.
  - `Visit(ImportStmtNode&)`에서 `loopDepth > 0`이면 에러.

### 3.3 Interpreter (`ExecutorUnit`) + ModuleLoader
- `ExecutorUnit::Visit(const ImportStmtNode&)`가 실행되는 시점에 `ModuleLoader`를 호출한다.
- `ModuleLoader`의 역할:
  1. 경로를 절대경로로 정규화.
  2. **파일 존재 확인** — 없으면 여기서 "import 대상 파일 없음" 에러.
  3. **순환 import 감지** — "현재 로딩 중인 파일 경로" 스택을 유지하다가, 스택에 이미 있는 경로를 다시 만나면 즉시 에러(예: `a.txt → b.txt → a.txt` 체인을 에러 메시지에 포함).
  4. 아직 로드한 적 없는 파일이면 읽어서 `Tokenizer` + `AssemblerUnit`으로 파싱(내부에 또 `import`가 있으면 재귀적으로 2~4번 반복).
  5. 파싱된 결과를 실행해 export(함수/전역 변수)를 추출하고, 이후 같은 파일을 다시 import할 때 재사용할 수 있도록 캐시.
- 실행 결과(export 맵)를 `ModuleObject` 값으로 감싸 `alias` 이름으로 **현재 스코프에만** 바인딩한다.

## 4. 이 정책으로 얻는 이점

- CheckerUnit이 파일 I/O 없는 순수 AST 분석기 성격을 그대로 유지한다 — 기존 단위 테스트 방식(손으로 AST 구성 후 `Check()` 호출)이 깨지지 않는다.
- 파일 관련 로직(존재 확인, 순환 감지, 파싱/캐싱)이 `ModuleLoader` 하나에 모여 있어 향후 변경(캐싱 전략, 상대/절대경로 정책 등)이 Checker나 Parser에 영향을 주지 않는다.
- Parse 단계와 Check 단계 사이에 별도의 정적 파이프라인 단계를 추가할 필요가 없다 — 기존 `Tokenizer → AssemblerUnit(Parse) → CheckerUnit(Check) → ExecutorUnit(Execute)` 구조를 그대로 유지하면서 `ModuleLoader`는 실행 시점에만 개입한다.

## 5. 오픈 이슈 현황

1. ~~import 대상 파일에서 `import`/`func` 선언/전역 `var` 선언 외의 구문이 나오면 무시할지 에러 낼지.~~ → **해결(2026-07-10, PR #102)**: 선언 외 구문은 무시하는 것으로 결정, `ExecutorUnit`이 실행 시점에 그렇게 처리한다. `CheckerUnit` 레벨의 명시적 검증은 아직 없음(필요하면 추가 가능).
2. ~~같은 파일을 여러 스코프에서 각각 import할 때, 매번 재실행할지 최초 1회만 실행하고 캐시된 export를 재사용할지.~~ → alias로 import할 때마다 매번 다시 실행해 `ModuleObject`를 새로 만든다(캐시 없음). 파일이 크거나 import 그래프가 넓으면 비용이 커질 수 있어 필요해지면 캐싱 도입 여지가 있음.
3. `sum.add(...)` 형태의 멤버 접근(`.`) 파싱 — **해결**: import 전용으로 좁히지 않고 클래스 기능의 `MemberAccessExprNode`를 그대로 공유한다. 대상이 `ModuleObject`인지 `InstanceObject`인지로 실행 시점에 분기.
4. 순환 import 에러 메시지에 전체 체인을 다 보여줄지, 마지막 두 파일만 보여줄지. → PR #76에서는 마지막 파일 하나(정규화된 경로)만 보여주는 방식으로 이미 구현됨(`"Circular import detected: <path> at line N"`). 전체 체인을 보여줄지는 여전히 오픈(급하지 않음).
5. ~~`alias`가 실제로 무엇을 하게 할지~~ → **해결(2026-07-10, PR #94/#96/#102)**: 6절에서 설명한 `ImportEnd` 마커 + `ModuleObject` 방식으로 alias 네임스페이싱이 실제로 구현됐다(옵션 (a)를 선택).

## 6. Import 실행부 — `ModuleObject` 기반 네임스페이스 (구현 완료)

> 이 절은 원래 "다 구현됐다면"이라는 가정 시나리오로 작성됐으나(2026-07-10 오전), 같은 날 PR #94/#96(`bd03407`)과 PR #102(`eabc281`)로 아래 설계가 그대로 구현됐다. 실제로 코드가 이렇게 동작하는지는 `ExecutorUnit.cpp`의 `Visit(const ImportStmtNode&)`로 확인할 수 있다.

오픈 이슈 5번을 **(a) alias 네임스페이싱**으로 결정했다(슬라이드 요구사항인 `math.add(1, 2)` 형태의 멤버 접근을 지원하기 위함).

### 6.1 파이프라인 최종 형태

- **Tokenizer**: 0절의 스플라이스 방식을 그대로 쓰되, 스플라이스 경계에 `ImportEnd` 마커 토큰을 삽입한다([Token.h](../Token.h)에 `TokenType::ImportEnd` 추가). 순환 import/파일없음 감지는 그대로 Tokenizer가 담당(변경 없음).
- **`ImportStatementParser`**: `import ... alias X;` 5개 토큰을 파싱한 뒤, `ImportEnd` 마커를 만날 때까지 이어지는 문장들을 계속 파싱해 `ImportStmtNode`의 자식(`children[2:]`)으로 흡수한다 — 스플라이스된 선언들이 더 이상 전역 최상위 토큰이 아니라 `ImportStmtNode.children` 아래에 있다.
- **CheckerUnit**: 기존 3.2절 체크(같은/상위 스코프 중복 import, alias 충돌, 반복문 내부 금지)는 그대로 유지.
- **`ExecutorUnit::Visit(const ImportStmtNode&)`**: 흡수된 하위 선언들을 별도 스코프에서 실행해 함수/전역 변수를 수집한 뒤, `ModuleObject`(alias 이름 → export 맵) 하나로 감싸 alias 식별자를 **현재 스코프에만** 바인딩한다. `MemberAccessExprNode`가 `ModuleObject`를 대상으로 오면 export 맵에서 이름을 찾아 호출/참조한다(클래스 인스턴스의 `.` 접근과 같은 코드 경로를 공유하되, 대상이 `ModuleObject`인 경우로 분기). alias 없이는 spliced된 이름이 바깥 스코프에 보이지 않는다 — 네임스페이스 격리가 실제로 검증됨.
- 파일 존재/순환 import는 여전히 Tokenizer가 가장 먼저 잡으므로, 3.3절에서 말한 실행 시점 `ModuleLoader`의 "파일 존재/순환 감지" 역할은 결국 도입되지 않았다.

### 6.2 남은 결정들

1. 같은 alias로 서로 다른 스코프에서 두 번 import하면 export 맵을 매번 새로 실행한다(캐시 없음) — 5절 이슈 2번 참고.
2. `ModuleObject`를 값으로 다른 변수에 대입하거나 함수 인자로 넘기는 것을 허용할지(1급 값 취급 여부) — 아직 미정.
3. export 대상을 명시적으로 선언(`export func add(...)`처럼)할지, 파일 내 모든 최상위 `func`/`var`를 암묵적으로 export할지 — 현재 구현은 후자(암묵적 전체 export)다.
