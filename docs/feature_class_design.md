# Class 기능 — CheckerUnit 구현 Study

## 0. 현재 상태

`Token.h`/`Tokenizer.cpp`에 `KwClass`/`KwThis`/`KwSuper`/`KwInstanceof`/`Dot`/`Colon` 토큰만 존재하고, AST 노드/파서/CheckerUnit/ExecutorUnit 전부 미착수(그린필드) 상태다. 이 문서는 CheckerUnit이 맡을 범위를 SRP 기준으로 정리한다.

## 1. 슬라이드 "클래스 관련 오류검사" 표를 SRP로 재분류

| 오류 | 슬라이드 표기 | 담당 | 이유 |
|---|---|---|---|
| 클래스 외부에서 `this` 사용 | (미표기) | **Checker** | `functionDepth`와 동일 패턴 — 구조적 위치 제약 |
| `init`에서 `return` 사용 | (미표기) | **Checker** | 이름이 `init`인 메서드 내부인지만 보면 됨 — 순수 구조 |
| 자기 자신 상속 (`Class Robot : Robot`) | (미표기) | **Checker** | 토큰 비교만으로 100% 확정 |
| 클래스가 아닌 대상 상속 (`Class Robot : x`) | (미표기) | **Checker** | "정의되지 않은 함수 호출" 체크와 동일 메커니즘(이름 registry 조회) |
| 클래스 외부에서 `Super` 사용 | (미표기) | **Checker** | this와 동일 |
| 부모 없는 클래스에서 `Super` 사용 | (미표기) | **Checker** | 클래스 선언 정보(부모 有無)만 보면 됨 — 순수 구조 |
| 인스턴스가 아닌 대상의 필드 접근 (`x.field`) | (미표기) | **Checker(리터럴만) / 나머지는 Interpreter** | 3번 항목 참고 — value-flow 트레이드오프 |
| 존재하지 않는 필드/메서드 접근 | **"런타임오류"로 명시됨** | **Interpreter** | 필드가 동적으로 생성되는 구조라 애초에 정적으로 알 방법이 없음 |

슬라이드에 "런타임오류"라고 명시된 건 마지막 줄 하나뿐이고, 나머지 7개는 표기가 없지만 전부 **컴파일 타임에 100% 확정 가능한 구조적 제약**이라 Checker 담당이 자연스럽다. 함수 기능에서 만든 패턴(`functionDepth`, `functionScopeStack`, "정의 안 된 함수 호출" 체크)을 거의 그대로 재사용할 수 있어 import보다 작업 범위가 명확하다.

## 2. 각 항목별 구현 아이디어 (기존 메커니즘 재사용 포인트)

- **`this`/`Super` 외부 사용 금지**: `functionDepth`처럼 `classMethodDepth`(또는 `insideClassBody`) 카운터 신설. 클래스의 메서드 body 진입/이탈 시 증감시키고, `this`/`Super` 노드 방문 시 0이면 에러.
- **`init`에서 `return` 금지**: 메서드 이름이 `"init"`일 때만 켜지는 플래그(`insideInit`) 추가. `Visit(ReturnStmtNode&)`에서 `insideInit`이면 에러 — 기존 "함수 밖 return 금지" 체크 옆에 나란히 추가.
- **자기 자신 상속**: `ClassDeclNode`의 자기 이름 토큰과 부모 이름 토큰을 문자열 비교만 하면 됨.
- **클래스 아닌 대상 상속 / instanceof 대상이 클래스인지**: `functionScopeStack`과 같은 패턴으로 `classScopeStack`(선언된 클래스 이름 → 부모 이름) 레지스트리 신설. 상속 대상 이름을 조회해서 없으면 에러(변수인지까지 구분해 "함수가 아니므로 호출 불가"류 메시지와 동일한 형태로 세분화 가능).
- **부모 없는 클래스에서 Super 사용**: 위 레지스트리에서 현재 클래스의 부모가 없으면, 그 클래스 메서드 내부의 `Super` 사용을 에러 처리.
- **인스턴스 아닌 대상 필드 접근**: 기존에 확정한 **"리터럴 기반 확정 오류만"** 원칙을 그대로 적용. `"hello".field = 1;`처럼 리터럴에 직접 접근하는 경우만 Checker가 잡고, 슬라이드 예시(`var x = "hello"; x.field = 1;`)처럼 변수를 거치는 경우는 Interpreter의 런타임 에러로 넘긴다 — 배열 기능에서 `var x = 10; x[0];`을 Checker에서 안 잡기로 한 결정과 동일한 축의 트레이드오프.

## 3. 슬라이드에 명시는 안 됐지만 검토할 만한 추가 항목

- **클래스 이름 중복 선언**: 변수/함수 중복 선언 체크와 동일 선상 — 슬라이드엔 없지만 빠뜨리면 어색함.
- **같은 클래스 내 메서드 이름 중복**: 함수 파라미터 중복 체크와 유사한 패턴.
- **`init` 호출 시 인자 개수 불일치** (`Robot("AndOr", 10)`): 이미 있는 `CallExprNode`의 "인자 개수 불일치" 체크를 `init`에도 적용 — `functionScopeStack`에 클래스의 `init` 파라미터 목록도 등록해두면 동일 코드로 커버 가능.

## 4. Checker가 동작하려면 AST/파서 쪽에 필요한 정보 (참고용 — Checker 담당 밖이지만 요구사항으로 명시)

- 클래스 선언 노드에 **자기 이름 + 부모 이름(없으면 nullopt)** 이 구조적으로 드러나야 함.
- `this`/`Super`가 각각 별도 노드 타입(`ThisExprNode`/`SuperExprNode`)으로 구분되어야 위치 제약 체크가 가능. (Import 논의에서 나온 `MemberAccessExprNode`의 좌변으로 쓰일 것으로 예상 — `this.name`, `Super.move(...)`)
- 인스턴스 생성(`Robot()`)이 일반 함수 호출(`CallExprNode`)과 문법이 동일하므로, Import에서 `Arr()`을 `ArrExprNode`로 따로 뽑았던 것처럼 클래스 이름 호출도 파서가 구분해서 별도 노드로 만들어줄지, 아니면 Checker/Executor가 이름으로 구분할지 파서 담당자와 사전 조율 필요.

## 5. 오픈 이슈 (팀 확인 필요)

1. 클래스 이름/메서드 이름 중복 체크를 슬라이드에 없어도 추가할지.
2. `init` 인자 개수 체크를 기존 함수 arg-count 체크와 통합할지, 별도로 둘지.
3. "인스턴스 아닌 대상 필드 접근"을 리터럴만으로 좁힐지, 변수 케이스까지 잡을지(범위를 넓히면 값-흐름 추적이 필요해져 기존 원칙과 충돌).
4. 클래스는 전역에서만 선언 가능한지, 함수처럼 중첩 스코프에서도 선언 가능한지(`classScopeStack`을 `functionScopeStack`처럼 스코프별로 둘지, 전역 하나로 둘지에 영향).
