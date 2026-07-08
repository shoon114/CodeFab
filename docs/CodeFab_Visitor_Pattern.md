# CheckerUnit/ExecutorUnit Visitor 패턴 구조

`CheckerUnit`/`ExecutorUnit`이 `SyntaxNode::type`을 switch/if-chain으로 직접
분기하던 방식을, `SyntaxNode`를 추상 베이스 + `NodeType`별 구체 서브클래스로
나누고 `NodeVisitor`를 통한 더블 디스패치로 교체한 구조를 정리한다.

## 1. 클래스 구조 (SyntaxNode 계층 + NodeVisitor)

```mermaid
classDiagram
    class SyntaxNode {
        <<abstract>>
        +NodeType type
        +Token token
        +vector~unique_ptr~SyntaxNode~~ children
        +Accept(NodeVisitor&) const*
    }

    class NodeVisitor {
        <<interface>>
        +Visit(const NumberLiteralNode&)
        +Visit(const VarDeclareStatementNode&)
        +Visit(const FuncDeclStmtNode&)
        +Visit(const ReturnStmtNode&)
        +Visit(const CallExprNode&)
        +Visit(const BlockStmtNode&)
        +... (17종)
        #Traverse(const SyntaxNode&) "기본 구현: 자식들을 그대로 순회"
    }

    SyntaxNode <|-- NumberLiteralNode
    SyntaxNode <|-- IdentifierNode
    SyntaxNode <|-- VarDeclareStatementNode
    SyntaxNode <|-- FuncDeclStmtNode
    SyntaxNode <|-- ReturnStmtNode
    SyntaxNode <|-- CallExprNode
    SyntaxNode <|-- BlockStmtNode
    SyntaxNode <|-- ProgramNode
    SyntaxNode ..> NodeVisitor : Accept(visitor)

    NodeVisitor <|.. CheckerUnit : override 일부만
    NodeVisitor <|.. ExecutorUnit : override 일부만
```

## 2. 더블 디스패치 흐름 (Accept → Visit)

```mermaid
sequenceDiagram
    participant Checker as CheckerUnit
    participant Node as FuncDeclStmtNode
    participant Visitor as NodeVisitor (base)

    Checker->>Node: node->Accept(*this)
    Note right of Node: Accept()는 컴파일 타임에<br/>자기 구체 타입을 알고 있음
    Node->>Checker: visitor.Visit(*this)  (오버로드 해석: FuncDeclStmtNode용)
    Note over Checker: CheckerUnit::Visit(FuncDeclStmtNode&)<br/>실행 (파라미터 중복검사, 스코프 등록,<br/>body->Accept(*this)로 재귀)
```

## 3. CheckerUnit의 함수 체크 흐름

예: 방금 확인한 `ExitScope()`가 쓰이는 지점.

```mermaid
flowchart TD
    A["Check(root)"] --> B["EnterScope() (Global)"]
    B --> C["root->Accept(*this)"]
    C --> D{"노드 타입은?"}
    D -->|"FuncDeclStmtNode"| E["파라미터 중복 검사<br/>함수를 현재 스코프에 등록"]
    E --> F["EnterScope()"]
    F --> G["파라미터를 변수로 등록"]
    G --> H["body->Accept(*this)"]
    H --> I["ExitScope()"]
    D -->|"BlockStmtNode"| J["EnterScope() → Traverse(node) → ExitScope()"]
    D -->|"VarDeclareStatementNode"| K["중복선언/자기참조 검사 → Traverse(node)"]
    D -->|"ReturnStmtNode"| L["functionDepth==0이면 오류 → Traverse(node)"]
    D -->|"CallExprNode"| M["함수/변수/미정의 여부 검사 → Traverse(node)"]
    D -->|"기타(기본 구현)"| N["Traverse(node): 자식만 순회"]
    I --> O["ExitScope() (Global)"]
```

## 핵심 요약

- `SyntaxNode`는 추상 클래스, `NodeType`마다 구체 서브클래스(17종)가 존재한다.
- `Accept()`가 자기 타입을 정확히 알고 `NodeVisitor::Visit()`의 올바른
  오버로드로 되돌려준다 (더블 디스패치).
- `NodeVisitor`의 기본 `Visit()`는 `Traverse()`로 자식을 그대로 훑기만 한다
  → `CheckerUnit`/`ExecutorUnit`은 관심 있는 타입만 override하면 된다.
- `ExitScope()`는 `FuncDeclStmtNode`/`BlockStmtNode` override 안에서 스코프를
  열고 닫는 지점에 쓰인다.
