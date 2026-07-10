# CheckerUnit/ExecutorUnit Visitor 패턴 적용 정리

대상 커밋: `7da2935` `[Feature][Refact]Checker Funciton 구현, GoF Visitor 패턴 적용 (#48)`
(같은 브랜치 내 개별 커밋 `027975a [refactor][SyntaxNode] AST 노드에 GoF Visitor 패턴 적용`이
master에는 PR #48로 스쿼시/머지되어 반영됨)

구조 자체에 대한 클래스/시퀀스 다이어그램은 `docs/CodeFab_Visitor_Pattern.md`에 이미 정리되어
있으므로, 이 문서는 "왜 이렇게 바꿨고 실제로 무엇이 좋아졌는지"에 초점을 맞춘다.

## 수정 사항

- `SyntaxNode`를 값 타입 단일 클래스에서 추상 베이스로 변경하고, `NodeType`마다 구체
  서브클래스(`NumberLiteralNode`, `VarDeclareStatementNode`, `FuncDeclStmtNode`,
  `ReturnStmtNode`, `CallExprNode`, `BlockStmtNode`, `ProgramNode` 등 17종)를
  `SyntaxNode.h`에 추가.
- `NodeVisitor.h`/`.cpp`를 신설: `NodeType` 17종 각각에 대해
  `virtual void Visit(const XxxNode&)`를 선언하고, 기본 구현은 `Traverse(node)`로
  자식 노드를 그대로 순회하도록 정의.
- `SyntaxNode`에 순수가상함수 `Accept(NodeVisitor&) const`를 추가하고, 각 구체
  서브클래스가 `Accept()`에서 `visitor.Visit(*this)`를 호출하도록 구현(더블 디스패치).
- `CheckerUnit`/`ExecutorUnit`이 기존에 `SyntaxNode::type`을 switch/if-chain으로
  직접 분기하던 코드를 제거하고, `NodeVisitor`를 상속받아 관심 있는 노드 타입만
  `Visit()`을 override하는 방식으로 전환. 진입점은 `root->Accept(*this)` 하나뿐.
- 이 변경과 함께 `CheckerUnit`에 함수(`FuncDeclStmtNode`/`ReturnStmtNode`/`CallExprNode`)
  체크 로직(중복 선언, 함수 밖 return, 인자 개수 불일치, 미정의 함수 호출 등)을 신규
  구현.
- 모든 파서(`VarDeclareParser`, `ExpressionParser`, `IfStatementParser`,
  `PrintStatementParser`, `BlockParser`)와 테스트 헬퍼가 `std::make_unique<SyntaxNode>()`
  대신 구체 노드 타입을 생성하도록 갱신. `TestExcutorUnit.cpp`는 트리를 직접 구성하는
  코드를 전면적으로 `unique_ptr<SyntaxNode>` 기반 구체 노드 생성 방식으로 재작성
  (추상 베이스는 값으로 만들 수 없으므로).

## 수정의 이유

- 기존 방식은 `CheckerUnit`/`ExecutorUnit` 각각이 `switch(node.type)` 또는
  `if/else` 체인으로 모든 `NodeType`을 알고 있어야 했다. 함수 체크 기능을 추가하면서
  `FuncDeclStmtNode`/`ReturnStmtNode`/`CallExprNode` 케이스가 늘어나자, 두 클래스
  모두에서 같은 분기 체인이 계속 길어지는 구조적 부담이 드러났다.
  - **Why**: 새 `NodeType`이 추가될 때마다 그 타입을 다루는 모든 순회 클래스의
    switch/if-chain을 일일이 찾아 수정해야 하고, 하나라도 빠뜨리면 컴파일 에러 없이
    조용히 해당 케이스가 기본 동작(또는 미정의 동작)으로 새는 위험이 있었다.
  - 이는 2절(Strategy + Registry)에서 파싱 단계의 `TokenType → IStatementParser`
    분기를 레지스트리로 뽑아내 Open-Closed Principle을 지킨 것과 같은 문제의식이며,
    "트리 순회 후 노드별 동작 결정"이라는 후속 단계에도 동일한 원칙을 적용한 것.

## 수정 후의 장점

- **관심사 분리**: `NodeVisitor`의 기본 `Visit()`가 `Traverse()`로 자식을 그대로
  순회하므로, `CheckerUnit`/`ExecutorUnit`은 실제로 반응해야 하는 노드 타입만
  override하면 된다. 관심 없는 타입은 코드를 한 줄도 쓸 필요가 없다.
- **컴파일 타임 안전성**: `Accept()`가 더블 디스패치로 정확한 구체 타입의 `Visit()`
  오버로드를 호출하므로, `dynamic_cast`나 `NodeType` 값 비교로 타입을 확인하던
  런타임 체크가 사라졌다.
- **확장 시 기존 코드 무변경(OCP)**: 새 `NodeType`을 추가해도 `AssemblerUnit`이
  다른 파서를 몰라도 되는 것과 마찬가지로, `NodeVisitor`의 기본 `Visit()`가 있으므로
  새 노드 타입 추가가 기존 `CheckerUnit`/`ExecutorUnit`이나 다른 Visitor를 건드릴
  필요가 없다(Open-Closed Principle).
- **테스트 안전성 향상**: `SyntaxNode`를 추상 베이스로 만들어 값 타입으로 생성하는
  것 자체가 불가능해졌기 때문에, TC에서 실수로 타입이 지정되지 않은 "빈"
  `SyntaxNode`를 만들어 검증 누락을 일으키는 경로가 원천적으로 차단됨.

## 기대효과

- 새 `NodeType`을 추가할 때 `CheckerUnit.cpp`/`ExecutorUnit.cpp`의 기존 분기 코드를
  건드리지 않고, 필요한 Visitor에만 새 `Visit()` override를 추가하면 되므로 회귀
  위험이 국소화될 것으로 기대했다.
- 관심 없는 Visitor는 기본 `Traverse()` 동작을 그대로 물려받으므로, 새 statement/expression
  기능이 아직 `CheckerUnit` 또는 `ExecutorUnit` 한쪽에서만 필요한 초기 구현 단계에서도
  다른 한쪽을 건드리지 않고 부분적으로 기능을 낼 수 있을 것으로 기대했다.
- 파서가 만드는 노드와 이를 소비하는 Visitor 사이의 매핑이 타입 시스템(오버로드
  해석)으로 강제되므로, "새 노드를 추가했는데 처리 코드를 깜빡함" 같은 실수가
  컴파일 경고/에러 없이 조용히 넘어가는 사례를 줄일 수 있을 것으로 기대했다.

## 실제로 수정 후에 기능 추가 시에 도움이 되었던 부분

Visitor 패턴 도입(#48) 이후 실제로 병합된 기능 커밋들에서 기대효과가 그대로 확인됐다.

- **`1d5bc9a` `[feature] Add static array (#54)`**: 새 표현식 기능을 추가하면서
  `SyntaxNode.h`(+27줄)에 새 노드 타입을 추가하고 `NodeVisitor.h`/`.cpp`에
  `Visit()` 기본 구현(+4/+2줄)만 얹었다. **`CheckerUnit.cpp`/`ExecutorUnit.cpp`는
  이 커밋에서 전혀 수정되지 않았다** — 새 노드가 별도 처리 없이 기존 `Traverse()`
  기본 동작만으로 정상 순회되었기 때문. 예상했던 "관심 없는 Visitor는 건드릴 필요
  없음"이 그대로 실현된 사례.
- **`bcbf5df` `[Feature] Import 기능 CheckerUnit 구현 (#80)`**: `ImportStmtNode`를
  추가하면서 `CheckerUnit.cpp`(+45줄)만 수정했고, `ExecutorUnit.cpp`는 이 시점에
  Import 실행 로직이 아직 필요 없어 **한 줄도 건드리지 않았다**. Checker와 Executor가
  서로 다른 시점에 각자의 속도로 같은 노드 타입을 지원하도록 확장할 수 있음을
  보여준 사례 — switch/if-chain 방식이었다면 두 클래스의 분기 테이블을 동시에
  맞춰야 한다는 압박이 있었을 것.
- **`674d649` `[Feature] Class 기능 CheckerUnit/ExecutorUnit 구현 및 테스트 추가 (#71)`**과
  **`8b8b872` `[Feature] CheckerUnit에 정적 바인딩/상수 폴딩 사전 계산 최적화 추가 (#53)`**
  역시 기존 `Visit()` 분기 코드를 되짚어 고치는 대신, 새 override 추가 또는 기존
  override 내부 로직 보강만으로 기능이 들어갔다 — 리팩토링 당시 기대했던
  "회귀 위험의 국소화"가 이후 4개 이상의 기능 브랜치에서 반복적으로 확인된 셈이다.
