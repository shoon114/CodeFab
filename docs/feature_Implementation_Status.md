# 3~4일차 기능 추가 요구사항 구현 현황

```mermaid
pie showData
    title 세부 항목 구현 상태 (표 기준 집계)
    "완료" : 40
    "부분(테스트 공백)" : 1
```

## 0. 문서 목적

`3일차_CodeFab Interpreter.pdf`(기능 추가 요청서)의 Chapter 2~7에 나열된 요구사항을
실제 코드(`*.cpp`/`*.h`)와 회귀 테스트(`Test*.cpp`)를 직접 열어서 대조한 결과를 정리한다.
"구현됨/구현 안 됨"을 단순 표기하는 게 아니라, **각 요구사항이 왜 그렇게 판단됐는지 근거
파일·근거 테스트**를 함께 남겨서, 나중에 코드가 바뀌었을 때 이 문서만 보고도 다시
검증할 수 있게 하는 것이 목적이다.

전체적으로 function/class/정적 배열/실행 전 최적화/import/공장 제어 쉘까지 6개
카테고리 전부 PDF 요구사항을 만족하는 상태다(2026-07-10 커밋 `7117b13` 기준, PR
#102/#103/#104/#105 반영). 남은 건 소규모 회귀 테스트 공백(파라미터 이름 중복,
`this.메서드()` 체이닝) 정도다.

---

## 1. function 관련 요구사항 (PDF Chapter 2)

### 요구사항
함수 선언·호출·매개변수 전달·재귀 호출을 지원하고, 함수 관련 4가지 오류
(외부 return, 파라미터 이름 중복, 비함수 호출, 인자 개수 불일치)를 검사한다.

### 세부 항목별 구현 상태

| 세부 항목 | 상태 | 근거 |
|---|---|---|
| 함수 선언 `Func add(a, b) { ... }` | 완료 | `FunctionStatementParser.cpp` |
| 함수 호출, 매개변수 전달 | 완료 | `ExecutorUnit::CallFunction` |
| `return;` → null, `return expr;` → 값 반환 | 완료 | `ExecutorUnit.cpp`(`ReturnSignal`/`ExecuteFunctionBody`), 테스트 `Execute_FuncCall_EmptyReturn_PrintsNull`, `Execute_FuncCall_FallsThroughWithoutReturn_PrintsNull` |
| 재귀 호출 | 완료 | 테스트 `Execute_FuncCall_Recursion_ComputesFactorial` |
| 함수 외부에서 `return` 사용 시 오류 | 완료 | `CheckerUnit::Visit(ReturnStmtNode)` + `Check_ReturnOutsideFunction_ReportsError` |
| 파라미터 이름 중복 시 오류 | 완료 | `CheckerUnit::Visit(FuncDeclStmtNode)`의 `seenParams` 검사 (전용 테스트는 못 찾았지만 로직은 존재 — 회귀 테스트 보강 여지 있음) |
| 함수가 아닌 대상 호출 시 오류 | 완료 | Checker의 `isVar` 분기 + Executor의 `'... is not callable'` 처리, 테스트 다수 |
| 인자 개수 불일치 시 오류 | 완료 | `Check_ArgumentCountMismatch_ReportsError`(정적) + `Execute_FuncCall_ArgumentCountMismatch_ThrowsRuntimeError`(런타임 이중 방어) |

### 종합 판단
**완료.** 유일한 보강 포인트는 "파라미터 이름 중복" 오류에 대응하는 전용 TC가 안 보인다는
것 — 로직 자체는 있으므로 기능 누락은 아니고 회귀 테스트 커버리지 공백 정도다.

---

## 2. class 관련 요구사항 (PDF Chapter 3)

### 요구사항
클래스 선언·인스턴스 생성, 필드 동적 읽기/쓰기, 메서드와 `this`, 생성자(`init`), 상속과
`Super`, `instanceof`, 그리고 이 전반에 걸친 오류 검사(8가지)를 지원한다.

### 세부 항목별 구현 상태

| 세부 항목 | 상태 | 근거 |
|---|---|---|
| 클래스 선언, 인스턴스 생성(`Robot()`) | 완료 | `ClassStatementParser.cpp` |
| 필드 동적 읽기/쓰기 | 완료 | `Execute_FieldWriteThenRead...` |
| 존재하지 않는 필드 읽기 → 런타임 오류 | 완료 | `Execute_ReadUndefinedField_ThrowsRuntimeError` |
| 메서드 선언/호출, `this`로 필드 접근 | 완료 | `ClassStatementParser.cpp`, `ExecutorUnit` |
| 메서드 내부에서 다른 메서드 호출(`this.report()`) | **부분** | `EvaluateMemberCall`이 `this`를 일반 target으로 처리해 동작은 되지만, 이 패턴만 전용으로 검증하는 테스트가 없음 — 회귀 안전망 공백 |
| 생성자 `init`(자동 호출, 인자 전달, 필드 초기화) | 완료 | `InstantiateClass`, `Execute_ConstructorInit_BindsArgsToFields` |
| `init`에서 `return` 사용 금지 | 완료 | `Check_ReturnInsideInit_ReportsError` |
| 상속, 메서드 오버라이딩, `Super` 호출 | 완료 | `Execute_SuperCall_InvokesParentMethodThenContinues` |
| `instanceof`(자기 자신 + 부모 클래스 모두 `true`) | 완료 | `Execute_Instanceof_TrueForSelfAndAncestor` |
| 클래스 외부 `this`/`Super` 사용, 자기 자신 상속, 클래스 아닌 대상 상속, 부모 없는 클래스에서 `Super` 사용 | 완료 | `CheckerUnit`의 구조적 제약 검사 (`feature_class_design.md`에서 설계했던 방향대로 구현됨) |
| 인스턴스 아닌 대상의 필드 접근 → 오류 | 완료 | `Execute_MemberAccessOnNonInstanceVariable_ThrowsRuntimeError` |
| 존재하지 않는 필드/메서드 접근 → 런타임 오류 | 완료 | `Execute_MethodCallOnNonInstance_ThrowsRuntimeError`, `Execute_CallUndefinedMethod_ThrowsRuntimeError` |

### 종합 판단
**거의 완료.** `docs/feature_class_design.md`에서 세워둔 설계 방향(SRP 기준으로 어떤 오류를
Checker가 잡고 어떤 걸 Interpreter가 잡을지)이 실제로 그대로 구현된 것으로 확인된다.
유일한 공백은 "메서드 안에서 `this.다른메서드()` 호출"에 대한 전용 테스트 부재.

---

## 3. 정적 배열 구현 (PDF Chapter 4)

### 요구사항
고정 크기 배열 생성(`Array(n)`)과 인덱스 읽기/쓰기(`arr[i]`), 그리고 4가지 런타임 오류
(범위 초과, 인덱스가 숫자 아님, 배열 아닌 대상에 `[]` 사용, 크기가 숫자 아님)를 지원한다.

### 구현 상태
**완료.** `Array(N)`은 전용 AST 노드(`ArrExprNode`)로, `arr[i]`는 `IndexExprNode`로
`ExpressionParser.cpp`에서 파싱된다. 4가지 런타임 오류는 모두
`ExecutorUnit::EvaluateArrExpr`/`ResolveIndexElement`에서 처리되며 각각 대응 테스트
(`Execute_IndexOutOfRange_*`, `Execute_ArrSize_NonNumber_ThrowsRuntimeError`,
`Execute_IndexingNonArrayValue_ThrowsRuntimeError` 등)가 존재한다. `CheckerUnit`도
리터럴로 100% 확정 가능한 경우(`Check_ArrSizeStringLiteral_ReportsError`,
`Check_InlineArrIndexOutOfRange_ReportsError`)는 정적으로 선제 검사하고 있어, 클래스
기능에서 세운 "리터럴은 Checker, 값-흐름이 필요한 나머지는 Interpreter" 원칙이 여기서도
일관되게 적용된 것으로 보인다.

---

## 4. 실행 전 최적화 (PDF Chapter 5)

### 요구사항
지역 변수 정적 바인딩(스코프 거리 사전 계산으로 동적 탐색 제거)과 상수 표현식 폴딩
(런타임 전에 100% 확정되는 리터럴 연산을 미리 계산)을 `CheckerUnit`에서 수행하고,
Test Double로 "실제로 최적화 경로를 탔는지"를 검증한다.

### 구현 상태
**완료.**

- **정적 바인딩**: `IdentifierNode::scopeDistance`를 `CheckerUnit::Visit(IdentifierNode)`가
  체크 시점에 계산해두고, `ExecutorUnit::ResolveVariable`이 실행 시점에 그 값을 그대로
  사용한다.
- **상수 폴딩**: `BinaryExprNode`/`UnaryExprNode`에 `isConstantFolded`/`foldedValue`
  필드를 두고, `CheckerUnit::Visit(Binary/UnaryExprNode)`에서 리터럴로만 구성된
  하위식을 판단해 미리 계산해둔다.
- **Test Double 검증**: PDF가 요구한 대로 "진짜로 최적화가 동작하는지"를 검증하는
  테스트가 존재한다.
  - `Execute_IdentifierWithStubbedScopeDistance_AccessesScopeDirectlyWithoutDynamicSearch`
    — 일부러 틀린 `scopeDistance`를 스텁해서, Executor가 동적 탐색으로 우회하지 않고
    스텁된 값을 그대로 믿고 접근하는지 확인.
  - `Execute_ConstantFoldedBinaryExpr_SkipsRecomputingPoisonedChildrenEachIteration`
    — 자식 노드를 일부러 오염(poison)시켜서, 폴딩된 노드가 매 반복마다 자식을
    재계산하지 않고 캐시된 `foldedValue`만 쓰는지 확인.
  - Checker 쪽도 `Check_ConstantArithmeticExpression_FoldsAtCheckTime`,
    `Check_ExpressionWithVariable_DoesNotFold`(변수가 섞이면 폴딩하지 않아야 함)로
    양성/음성 케이스를 모두 커버.

---

## 5. import 관련 요구사항 (PDF Chapter 6)

### 요구사항
`import "경로" alias 별칭;` 문법, 반복문 내부 import 금지, import 대상 파일은 선언만
허용, 경로는 문자열 리터럴 강제, 순환 import 시 오류, import는 실행된 scope에만
적용(상위 scope 재-import 금지, 같은 scope 재-import 금지), 그리고 이와 관련한 6가지
오류(문법 오류/파일 없음/같은 scope 중복/순환 import/alias 충돌/반복문 내 사용)를
검사한다.

### 세부 항목별 구현 상태

| 세부 항목 | 상태 | 근거 |
|---|---|---|
| `import "경로" alias 별칭;` 문법 | 완료 | `ImportStatementParser.cpp` |
| 문자열 리터럴 강제, 문법 오류 검사 | 완료 | `ImportStatementParser.cpp` 파싱 실패 시 `std::runtime_error` |
| 반복문 내부 import 금지 | 완료 | `CheckerUnit::Visit(ImportStmtNode)`, `Check_ImportInsideForLoop_ReportsError` |
| alias 이름 충돌 | 완료 | 변수 중복 선언 검사 재사용, 관련 테스트 존재 |
| 같은 scope 내 중복 import | 완료 | `Check_DuplicateImportInSameScope_ReportsError` |
| 상위(조상) scope에서 이미 import된 파일 재-import 금지 | 완료 | `Check_ReimportFileAlreadyImportedInAncestorScope_ReportsError` |
| 실제 파일 시스템에서 파일을 찾아 읽고 토큰화 | 완료 | `Tokenizer::ReadFile`이 `std::ifstream`으로 실제 파일을 열고, `Tokenizer::ResolveImports`가 `import` 문을 만나면 `TokenizeFileForImport`로 그 파일을 실제로 토큰화해서 import 문 바로 뒤에 이어붙인다(재귀 지원). `TestTokenizerUnit.cpp`의 `CreateTokenForCode_ImportSplicesImportedFileTokensRightAfterStatement`, `CreateTokenForCode_ImportRecursivelySplicesNestedImports`로 검증됨 |
| import 대상 파일 없음 → 오류 | 완료 | `Tokenizer::ReadFile`이 파일을 못 열면 `"Cannot open import file: <path> at line N"`을 던짐. `CreateTokenForCode_ImportOfMissingFileThrows`로 검증 |
| 순환 import(A→B→A, 서로 다른 두 파일) 감지 | 완료 | `Tokenizer::TokenizeFileForImport`가 `activeImports` 스택에 정규화된 경로(`std::filesystem::weakly_canonical`)로 감지하며, 걸리면 `"Circular import detected: ..."`를 던짐. `CreateTokenForCode_CircularImportThrows`, 경로 표기가 달라도(`./` 섞임) 잡아내는 `CreateTokenForCode_CircularImportThroughDifferentlyFormattedPathStillThrows`로 검증 |
| alias 네임스페이스로 멤버 접근(`sum.add(1, 2)`) | 완료 | `ExecutorUnit::Visit(const ImportStmtNode&)`가 import 대상 파일의 선언들(`node.children[2:]`)을 별도 스코프에서 실행한 뒤 `ModuleObject`로 감싸 alias 이름에 바인딩한다. `.` 접근은 `InvokeModuleFunction`/멤버 조회가 `ModuleObject`를 처리해 실제로 `math.add(...)`처럼 동작한다. `Execute_ImportedModuleFunctionCall_ReturnsSum`, `Execute_ImportedModuleVariableRead_ReturnsValue`, `Execute_ImportedModuleFunction_NotVisibleWithoutAlias`(alias 없이는 안 보임 = 네임스페이스 격리가 실제로 동작함), `Execute_ImportedModuleUndefinedFunctionCall_ThrowsRuntimeError`로 검증 |
| import 스플라이스 경계를 정확히 구분(뒤이은 사용자 코드와 안 섞임) | 완료 | `Token.h`에 `ImportEnd` 마커 토큰이 추가됐고, `Tokenizer::ResolveImports`가 스플라이스한 내용 끝에 이 마커를 심는다. `ImportStatementParser::Parse`가 이 마커를 만날 때까지만 문장을 계속 파싱해 `ImportStmtNode.children`으로 흡수한다. `Parse_ImportEndBoundary_ParsesModuleContentIntoChildren`, `Parse_NestedImportBeforeImportEnd_ConsumesEachBoundarySeparately`로 검증 |
| import 대상 파일은 선언(import/함수선언/전역 var선언)만 허용, 그 외 구문 무시 | 완료 | PDF가 허용한 두 정책("에러 처리" 또는 "선언 외 ignore") 중 **ignore**로 결정됐다. `ExecutorUnit`에 `suppressActionsDuringImport` 플래그를 두고, import 모듈 내용을 실행하는 동안 `Visit(PrintStmtNode)`/`Visit(BlockStmtNode)`/`Visit(IfStmtNode)`/`Visit(ExprStmtNode)`/`Visit(ForStmtNode)`가 스스로 그 플래그를 보고 아무 것도 안 하고 리턴한다(선언 계열 Visit는 이 체크가 없어 그대로 실행됨). `Execute_ImportedModuleContainingPrintStmt_DoesNotExecutePrint`로 검증. `CheckerUnit::Visit(ImportStmtNode)`도 `EnterScope()`/`ExitScope()`로 감싸 이 선언들을 독립 스코프에서 정적 검사한다(`Check_ImportedVarNameMatchesOuterVar_ReturnsTrue`, `Check_DuplicateVarInsideImportedModule_ReportsError`) |
| import 대상 파일 없음 오류가 REPL/파일 모드에서 실제로 잡히는지 | 완료 | 이전에는 `PromptMode`/`FileMode`가 토큰화(`CreateTokenForCode`/`Tokenize`)를 try 블록 **밖에서** 호출해서, import 파일이 없을 때 `Tokenizer`가 던지는 예외가 새지 않을지 불확실했다. 이제 두 곳 다 토큰화를 try 블록 **안으로** 옮겨 정상적으로 catch하고 에러 메시지를 출력한 뒤 계속 진행하도록 고쳐짐 |
| `SourceFileLoader`와의 관계 | 참고 | `SourceFileLoader`는 `FileMode`/`DebugMode`가 **진입점 소스 파일 하나**를 읽는 용도로만 쓰이고, import 대상 파일을 읽는 `Tokenizer::ReadFile`과는 별개의 코드 경로 — 이름이 비슷해서 혼동하기 쉬우니 유의. |

### 종합 판단
import는 파일을 찾아 읽고 토큰화해서 alias 네임스페이스(`ModuleObject`)로 감싸는
것, 스플라이스 경계를 `ImportEnd` 마커로 명확히 구분하는 것, import 대상 파일에서
선언 외의 구문(print/if/for/단독 식)은 조용히 무시하는 것까지 전부 구현·테스트돼
있다. PDF Chapter 6의 요구사항을 전부 만족한 상태로 보인다. 자세한 파이프라인
흐름은 `docs/feature_Import_design.md` 0절 참고.

---

## 6. 공장 제어 쉘 (PDF Chapter 7)

### 요구사항
Interpreter Factory를 실행하는 3가지 모드(프롬프트/파일/디버그)를 지원한다.

### 세부 항목별 구현 상태

| 세부 항목 | 상태 | 근거 |
|---|---|---|
| 모드 분기(`prompt`/`run`/`debug`) | 완료 | `CodeFabInterpreter.cpp::SelectMode` |
| 프롬프트 모드(REPL, `exit`/`quit` 종료) | 완료 | `PromptMode.cpp` |
| 파일 모드(`run <경로>`) | 완료 | `FileMode.cpp` — 파일 없으면 "File not found" 오류, 런타임 오류 시 "at line N" 포함 메시지 출력 후 종료 코드 1 반환 |
| 디버그 모드 아키텍처(ExecutorUnit과 분리) | 완료 | GoF **Observer 패턴**으로 구현됨. `ExecutionObserver` 인터페이스(`OnStmtEnter`/`OnStmtExit`)를 두고 `ExecutorUnit::SetObserver`로 주입 — observer가 `nullptr`이면(prompt/run 모드) 기존 동작이 그대로 유지된다. `DebugSession`이 이 인터페이스를 구현해 문 경계마다 통보받는다 |
| watch/unwatch/watches/inspect | 완료 | `WatchList`가 담당. Number/Boolean/String/Array 타입 watch, 선언 안 된 변수는 `<undefined>` 표시, local/global 스코프 구분 캡처, 현재 스코프의 변수만 보여주는 `inspect`까지 `TestWatchList.cpp`(435줄)로 검증됨 |
| step(항상 정지)/next(현재 depth 이하로 돌아올 때까지 진입 X)/continue(breakpoint까지 실행) | 완료 | `StepController`가 `Mode{Stepping, SteppingOver, Running}` 상태 머신으로 구현됨. `Step()`→`Stepping`(항상 `ShouldPause` true), `Next(depth)`→`SteppingOver`(현재 depth 기록 후 `depth <= stepOverDepth`일 때만 true), `Continue()`→`Running`(breakpoint 줄에서만 true). `TestStepController.cpp` 19개 케이스(`Step_PausesAtEveryStatement`, `Next_PausesWhenDepthAtOrBelowRecorded`/`Next_SkipsWhenDepthAboveRecorded`, `Continue_PausesAtBreakpointLine`/`Continue_DoesNotPauseAtNonBreakpointLine`, `ModeTransition_*` 등)로 검증 |
| break/remove/breakpoints(줄 번호 설정·해제·목록) | 완료 | `StepController::SetBreakpoint`/`RemoveBreakpoint`/`Breakpoints()`(내부는 0-indexed, 사용자 입출력은 1-indexed로 `DebugSession`이 변환). `SetBreakpoint_AppearsInBreakpoints`, `SetBreakpoint_DuplicateIgnored`, `RemoveBreakpoint_RemovesExisting`/`RemoveBreakpoint_NonExistentIsNoOp`로 검증 |
| 명령어 파싱·라우팅 + `exit`/`quit`로 디버그 모드 종료 | 완료 | `DebugSession::HandleCommand`가 step/next/continue/break/remove/breakpoints/watch/unwatch/watches/inspect/exit/quit 전부를 처리한다 |

### 종합 판단
프롬프트/파일/디버그 모드 전부 **완료**. 디버그 모드는 Observer 패턴으로
ExecutorUnit과 분리된 아키텍처 위에 `StepController`(언제 멈출지)와
`WatchList`(멈췄을 때 뭘 보여줄지)가 역할을 나눠 구현돼 있고, 각각 전용 테스트
(`TestStepController.cpp`, `TestWatchList.cpp`)로 검증됐다. PDF Chapter 7의
프롬프트/파일/디버그 요구사항을 전부 만족한 상태다.

---

## 7. 남은 작업 우선순위 정리

| 우선순위 | 작업 | 이유 |
|---|---|---|
| 1 | 회귀 테스트 공백 보강 (파라미터 이름 중복, `this.메서드()` 체이닝) | 기능 자체는 동작하므로 급하지 않지만, 리팩토링 시 회귀를 못 잡을 위험이 있음 |

6개 카테고리 요구사항이 전부 구현된 상태라 남은 작업은 이 회귀 테스트 공백
하나뿐이다(맨 위 차트 참고).
