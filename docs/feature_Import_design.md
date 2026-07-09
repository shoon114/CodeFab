# Library Import 기능 — 에러 처리 책임 분담 정책

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

`feature/CheckerImport`에서 이미 구현한 CheckerUnit 쪽(2~3.2절 내용)은 **코드 레벨 충돌은 없다** — PR #76은 Tokenizer만 건드렸고, `ImportStmtNode`/`ImportStmtParser`는 아직 아무도 만들지 않았다. 다만 아래를 유의해야 한다.

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

## 5. 오픈 이슈 (미결정)

1. import 대상 파일에서 `import`/`func` 선언/전역 `var` 선언 외의 구문이 나오면 무시할지 에러 낼지.
2. ~~같은 파일을 여러 스코프에서 각각 import할 때, 매번 재실행할지 최초 1회만 실행하고 캐시된 export를 재사용할지.~~ → Tokenizer 스플라이스 방식에서는 "실행"이라는 개념 자체가 없고 매번 파일 내용을 텍스트로 이어붙이므로 이 이슈는 현재 아키텍처에서는 의미가 없어졌다(0절 참고). 다만 같은 파일이 여러 곳에서 반복 import되면 매번 파일을 다시 읽고 재귀 토큰화하므로, 파일이 크거나 import 그래프가 넓으면 토큰화 비용이 커질 수 있다 — 필요해지면 캐싱을 Tokenizer나 후속 ModuleLoader 쪽에 도입.
3. `sum.add(...)` 형태의 멤버 접근(`.`) 파싱을 import 전용으로 좁게 만들지, 클래스 기능(`this.x`)과 공유하는 범용 노드로 설계할지.
4. 순환 import 에러 메시지에 전체 체인을 다 보여줄지, 마지막 두 파일만 보여줄지. → PR #76에서는 마지막 파일 하나(정규화된 경로)만 보여주는 방식으로 이미 구현됨(`"Circular import detected: <path> at line N"`). 전체 체인을 보여줄지는 여전히 오픈.
5. **(신규, 가장 중요)** `alias`가 실제로 무엇을 하게 할지 아직 정해지지 않았다. 현재 Tokenizer 스플라이스 방식은 alias를 완전히 무시하고 대상 파일 내용을 네임스페이스 없이 그대로 펼친다. 슬라이드가 요구하는 `math.add(1, 2)`처럼 alias를 통한 멤버 접근을 지원하려면 다음 중 하나가 필요하다:
   - (a) Parser가 `import ... alias X;` 뒤에 이어지는 스플라이스된 선언들을 어디까지인지 구분해 `X`라는 이름의 네임스페이스로 묶어주는 별도 처리를 추가한다(현재 토큰 스트림만 봐서는 "여기까지가 이 파일에서 온 선언"이라는 경계 정보가 토큰에 남아 있지 않아, 이 경계를 표시할 방법 — 예: 시작/끝 마커 토큰 — 을 Tokenizer가 추가로 남겨줘야 할 수 있다).
   - (b) alias 네임스페이싱을 포기하고, import된 함수/변수를 그냥 전역(또는 import 문이 위치한 스코프)에 평범한 이름으로 노출하는 것으로 요구사항을 재정의한다(이 경우 `math.add(...)` 대신 `add(...)`처럼 그냥 호출).
   이 결정이 나기 전까지 CheckerUnit의 "alias 이름 충돌" 체크는 유지하되(현재 스코프에 선언되는 이름이라는 점은 (a)/(b) 어느 쪽이든 참이므로), MemberAccessExprNode 관련 로직과 연결할지는 보류.
