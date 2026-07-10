# 테스트 현황: UnitTC / SystemTC / 커버리지

> 발표 원본 자료. 그래프(`docs/presentation/test_coverage.svg`)는 이 문서의
> 커버리지 수치를 막대그래프로 시각화한 것이다.

## 1. Unit TC (모듈별)

gmock/gtest 기반 유닛 테스트. `TEST`/`TEST_F` 개수 기준.

| 분류 | 모듈 (파일) | 개수 |
|---|---|---|
| 어휘 분석 | Tokenizer | 90 |
| 파서 코어 | AssemblerUnit + StatementParserRegistry | 6 |
| 개별 Statement 파서 | VarDeclareParser | 6 |
| | PrintStatementParser | 4 |
| | ReturnStatementParser | 4 |
| | BlockParser | 5 |
| | IfStatementParser | 15 |
| | ForStmtParser | 10 |
| | FunctionStatementParser | 8 |
| | ClassStatementParser | 7 |
| | ImportStatementParser | 5 |
| | ExpressionParser | 44 |
| | **파서 소계** | **108** |
| 정적 검사 | CheckerUnit | 51 |
| 실행 | ExecutorUnit | 74 |
| **전체 합계** | | **329** |

## 2. System TC (기능별)

`.claude/skills/system-test/run.ps1`에 정의된, 콘솔 입력 → 실행 결과를
검증하는 system test. `Category` 항목 기준.

| 기능 | 개수 |
|---|---|
| 표현식/연산자·출력 포맷 | 13 |
| 변수/스코프 | 8 |
| 제어문 if/else/for | 20 |
| 함수 | 10 |
| 배열 | 9 |
| 클래스 | 9 |
| for 스코프 누수 | 2 |
| 구문 오류 일반 | 4 |
| 타입 오류 | 2 |
| **전체 합계** | **77** |

## 3. UnitTC + SystemTC를 총동원한 최종 라인 커버리지

Debug 빌드(gtest/gmock 329개 실행)와 Release 빌드(system test 77개 케이스
개별 실행)를 [OpenCppCoverage](https://github.com/OpenCppCoverage/OpenCppCoverage)로
각각 계측한 뒤, 파일(모듈) 단위로 라인 히트를 합집합으로 병합해서 계산했다
(한쪽에서만 실행됐어도 그 줄은 "커버됨"으로 집계).

**전체 평균 라인 커버리지: 89.5% (1478/1652줄)**

| 모듈 | 커버리지 | 커버된 줄 |
|---|---|---|
| Tokenizer.cpp | 100.0% | 211/211 |
| StatementParserRegistry.cpp | 100.0% | 13/13 |
| VarDeclareParser.cpp | 100.0% | 16/16 |
| PrintStatementParser.cpp | 100.0% | 15/15 |
| ReturnStatementParser.cpp | 100.0% | 17/17 |
| IfStatementParser.cpp | 100.0% | 49/49 |
| ForStmtParser.cpp | 100.0% | 40/40 |
| ImportStatementParser.cpp | 100.0% | 25/25 |
| ExpressionParser.cpp | 99.3% | 149/150 |
| FunctionStatementParser.cpp | 97.1% | 33/34 |
| BlockParser.cpp | 95.5% | 21/22 |
| AssemblerUnit.cpp | 94.7% | 18/19 |
| ExecutorUnit.cpp | 90.5% | 408/451 |
| CheckerUnit.cpp | 90.4% | 312/345 |
| ClassStatementParser.cpp | 89.1% | 49/55 |
| PromptMode.cpp | 78.0% | 71/91 |
| CodeFabInterpreter.cpp | 55.6% | 10/18 |
| NodeVisitor.cpp | 44.8% | 13/29 |
| SourceFileLoader.cpp | 30.8% | 8/26 |
| FileMode.cpp | 0.0% | 0/20 |
| DebugMode.cpp | 0.0% | 0/6 |

### 눈에 띄는 지점

- **파서 계열(파서 코어 + 개별 statement 파서)은 대부분 90~100%** — 유닛
  테스트가 가장 촘촘하게 쌓인 영역이라는 게 수치로도 드러난다.
- **`FileMode.cpp`/`DebugMode.cpp`는 0%.** 우리 TC는 전부 `PromptMode`(콘솔
  REPL, system test) 또는 Debug exe의 `RUN_ALL_TESTS()`(unit test) 경로만
  실행하고, `run <file>`/`debug <file>` 진입점은 어느 쪽도 거치지 않기
  때문이다. 실제 기능은 동작하지만(수동으로는 확인됨), 자동화된 테스트로는
  전혀 검증되지 않고 있다는 뜻 — 향후 보강이 필요한 지점으로 발표에서
  솔직하게 언급할 만하다.
- **`NodeVisitor.cpp` 48.3%.** 기본 `Visit()` 구현(관심 없는 Visitor가
  기본 순회로 넘어가는 코드)들 중 실제로 트리거되는 조합이 제한적이라
  낮게 나온다 — Visitor 패턴 특성상 "각 노드 타입 × 각 Visitor"의 모든
  조합을 다 태우기는 어렵다는 걸 보여주는 사례.
- **`SourceFileLoader.cpp` 30.8%.** `import`를 통한 다른 파일 로딩 경로
  일부(예: 파일을 못 찾는 에러 처리 등)가 system test에서 충분히
  다뤄지지 않고 있다는 뜻.
