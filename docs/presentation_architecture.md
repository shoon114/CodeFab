# CodeFab 아키텍처

> 발표 원본 자료. 슬라이드(`docs/presentation/index.html`)는 이 문서를 축약한 것이다.

## 1. 전체 파이프라인

```text
Source Code
  -> Tokenizer            (문자열 -> TokenList)
  -> AssemblerUnit         (TokenList -> SyntaxNode AST)
  -> CheckerUnit           (AST 정적 검사 + 정적 바인딩/상수 폴딩)
  -> ExecutorUnit          (AST 실행)
  -> Runtime Result
```

REPL(`PromptMode`)은 한 줄씩 입력을 누적하다가 "완결된 statement"로 판단되는
시점에만 이 파이프라인 전체를 한 번 돌린다. 실행 모드는 `ShellMode`를
상속한 `PromptMode`/`FileMode`/`DebugMode` 세 가지로 나뉘어 있고, 진입점
`main()`은 인자에 따라 어떤 모드로 들어갈지만 결정한다(전략 패턴).

## 2. Parser: Strategy + Registry

`AssemblerUnit::Parse()`는 각 statement 타입을 switch/if-else로 직접
분기하지 않는다. 모든 파서는 단일 인터페이스(`IStatementParser`)로
통일되어 있고, 전역 레지스트리(`StatementParserRegistry`)를 통해
런타임에 필요한 파서를 얻어와 위임한다.

```text
AssemblerUnit
  -> tokenList[pos].type 확인
  -> StatementParserRegistry::Instance().Resolve(tokenType)
  -> IStatementParser::Parse(tokenList, pos)
  -> SyntaxNode 반환
```

- **자기 등록(self-registration)**: 새 파서를 추가해도 `AssemblerUnit`이나
  다른 파서의 코드를 수정할 필요가 없다. `StatementParserRegistrar<T>`
  템플릿으로 각 파서의 `.cpp` 파일에 한 줄만 추가하면 자기 자신을
  registry에 등록한다.
- **생성자 주입 금지**: 어떤 파서도 다른 파서를 생성자로 주입받지 않는다.
  필요한 하위 파서는 항상 실행 시점에 `Resolve()`로 얻어온다.
- 이 구조 덕분에 `for`/`if`/`func`/`Class`의 body를 파싱할 때도
  "이게 어떤 종류의 문장인지"를 `ForStmtParser` 등이 전혀 몰라도 되고,
  leading token만 보고 위임하면 된다. 실제로 `for (...) print j;`처럼
  `{}` 없는 단일 문장 body를 지원하도록 확장할 때도 `ForStmtParser` 내부
  로직만 바뀌었을 뿐 다른 파서는 전혀 손대지 않았다.

## 3. AST 순회: GoF Visitor 패턴

`CheckerUnit`/`ExecutorUnit`이 `SyntaxNode::type`을 switch/if-chain으로
직접 분기하던 초기 설계는 더블 디스패치 기반 Visitor 패턴으로 교체됐다.

- `SyntaxNode`: 추상 베이스. `NodeType`마다 구체 서브클래스
  (`IfStmtNode`, `ForStmtNode`, `ClassDeclStmtNode`, `ImportStmtNode` 등)가
  존재하며, 각자 `Accept(NodeVisitor&)`에서 `visitor.Visit(*this)`를
  호출해 자신의 정확한 타입으로 되돌려준다.
- `NodeVisitor`: `NodeType` 값마다 하나씩 `virtual void Visit(const XxxNode&)`를
  선언한 인터페이스. 기본 구현은 `Traverse(node)`로 자식을 그대로
  순회하는 것이므로, 구체 Visitor(`CheckerUnit`, `ExecutorUnit`)는
  관심 있는 노드 타입만 override하면 된다.
- 이 패턴 덕분에 새 노드 타입(`ClassDeclStmtNode`, `ImportStmtNode` 등)이
  추가돼도, 그 노드에 반응할 필요가 없는 기존 Visitor는 코드를 전혀
  수정하지 않아도 기본 순회 동작으로 자연스럽게 넘어간다.

## 4. 스코프/런타임 모델

- `ExecutorUnit::scopes`: `vector<unordered_map<string, Value_t>>` 스택.
  블록(`{}`)에 진입할 때마다 새 프레임을 push한다.
- `ScopeGuard`(RAII): 생성 시 `EnterScope()`, 소멸 시 `ExitScope()`를
  보장한다. 예외(특히 `return`을 표현하는 내부 `ReturnSignal` 예외)가
  중간에 던져져도 스코프가 항상 짝을 맞춰 정리된다.
- `CheckerUnit`은 별도의 `scopeStack`을 정적으로 유지하며, 변수 사용
  시점에 선언된 스코프까지의 거리(`IdentifierNode::scopeDistance`)를
  미리 계산해둔다. **이 두 스코프 모델(Checker의 정적 스택, Executor의
  런타임 스택)은 프레임 수가 정확히 일치해야 하며, 어긋나면 잘못된
  변수를 참조하는 조용한 버그가 생긴다** — 실제로 `for` 문의 스코프를
  다룰 때 이 동기화가 깨져서 발견/수정한 사례가 있다(뒤 슬라이드의
  "잘 처리한 예외 케이스" 참고).

## 5. 클래스/상속 모델

- 인스턴스(`InstanceObject`)는 필드를 `unordered_map<string, Value_t>`로
  동적으로 보관한다(선언 없이 `this.field = value`로 생성).
- 메서드 탐색은 클래스 이름 -> 부모 클래스 이름 체인을 타고 올라가며
  찾는다(`FindMethod`).
- `Super.method(...)`는 "인스턴스의 실제 클래스"가 아니라 "현재 실행
  중인 메서드가 정의된 클래스의 부모"부터 탐색을 시작해야 오버라이딩한
  메서드를 다시 호출하는 무한 재귀에 빠지지 않는다.
