# Library Import 기능 — 에러 처리 책임 분담 정책

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
2. 같은 파일을 여러 스코프에서 각각 import할 때, 매번 재실행할지 최초 1회만 실행하고 캐시된 export를 재사용할지.
3. `sum.add(...)` 형태의 멤버 접근(`.`) 파싱을 import 전용으로 좁게 만들지, 클래스 기능(`this.x`)과 공유하는 범용 노드로 설계할지.
4. 순환 import 에러 메시지에 전체 체인을 다 보여줄지, 마지막 두 파일만 보여줄지.
