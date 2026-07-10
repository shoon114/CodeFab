#include "CheckerUnit.h"
#include <cmath>
#include <iostream>
#include <iterator>

bool CheckerUnit::Check(SyntaxNode* root) {
    if (!root) return true;
    hasError = false;
    functionDepth = 0;
    classMethodDepth = 0;
    insideInit = false;
    classHasParentStack.clear();
    loopDepth = 0;
    if (scopeStack.empty()) {
        // Global 스코프(최초 1회만 생성 — REPL에서 이전 줄에 선언한 변수/함수/클래스/import를
        // 이후 줄에서도 계속 참조할 수 있어야 하므로, ExecutorUnit::Execute()가 scopes[0]을
        // 유지하는 것과 동일한 이유로 여기서도 매 Check() 호출마다 초기화하지 않는다. 그렇지
        // 않으면 "Func add(a, b) {...}"와 "print add(1, 2);"가 서로 다른 줄(= 서로 다른
        // Check() 호출)로 들어올 때 add가 미등록 함수로 오인되어 거짓 오류가 발생한다.
        EnterScope();
    }
    root->Accept(*this);
    return !hasError;
}

void CheckerUnit::Visit(const VarDeclareStatementNode& node) {
    // VarDeclareStatementNode 자신의 token은 항상 'var' 키워드다. 실제 변수
    // 이름은 children[0]에 있는데(VarDeclareParser 참고), 초기화식이 있으면
    // children[0]이 AssignExpr(Identifier, initializer)이고, 없으면
    // children[0]이 Identifier 노드 하나뿐이다.
    const SyntaxNode& declared = *node.children[0];
    const SyntaxNode* identifierNode = &declared;
    const SyntaxNode* initializerNode = nullptr;
    if (declared.type == NodeType::AssignExpr) {
        identifierNode = declared.children[0].get();
        initializerNode = declared.children[1].get();
    }
    const std::string& varName = identifierNode->token.lexeme;
    int line = identifierNode->token.line;

    // 1. 중복 선언 검사 (현재 스코프만 확인)
    if (scopeStack.back().count(varName)) {
        ReportError("변수 '" + varName + "' 중복 선언.", line);
    }

    // 2. 자기 참조 검사: 초기화식이 있는 경우에만 그 안에서 자기 자신을
    // 참조하는지 확인한다(초기화식이 없으면 검사할 대상이 없다).
    if (initializerNode != nullptr) {
        if (IsReferencingVar(initializerNode, varName)) {
            ReportError("자신의 초기화식에서 변수 '" + varName + "'을(를) 참조할 수 없습니다.", line);
        }
    }

    // 3. 검사 후 현재 스코프에 등록
    scopeStack.back().insert(varName);

    // 4. 초기화식 내부(중첩 호출 등)도 계속 검사
    Traverse(node);
}

void CheckerUnit::Visit(const BlockStmtNode& node) {
    EnterScope();
    Traverse(node);
    ExitScope();
}

void CheckerUnit::Visit(const FuncDeclStmtNode& node) {
    const std::string& funcName = node.token.lexeme;

    if (node.children.empty()) {
        ReportError("함수 '" + funcName + "'에 본문이 없습니다.", node.token.line);
        return;
    }

    // 마지막 child가 body(BlockStmt), 그 앞은 파라미터(Identifier) 목록이라고 가정
    const SyntaxNode* body = node.children.back().get();

    std::vector<std::string> paramNames;
    std::unordered_set<std::string> seenParams;
    for (size_t i = 0; i + 1 < node.children.size(); ++i) {
        const std::string& paramName = node.children[i]->token.lexeme;
        if (seenParams.count(paramName)) {
            ReportError("함수 '" + funcName + "'의 파라미터 이름 '" + paramName + "'이(가) 중복되었습니다.", node.token.line);
        }
        seenParams.insert(paramName);
        paramNames.push_back(paramName);
    }

    // 함수 이름 중복 선언 검사 (현재 스코프만 확인) — 아래 등록 이전에 검사해야
    // 두 번째 선언이 첫 번째를 조용히 덮어쓰기 전에 잡을 수 있다.
    if (functionScopeStack.back().count(funcName)) {
        ReportError("함수 '" + funcName + "' 중복 선언.", node.token.line);
    }

    // 함수는 선언된 스코프(자기 자신을 포함해 재귀 호출이 가능하도록 body 진입 전)에 등록
    functionScopeStack.back()[funcName] = paramNames;

    functionDepth++;
    EnterScope();
    for (const auto& paramName : paramNames) {
        scopeStack.back().insert(paramName);
    }
    body->Accept(*this);
    ExitScope();
    functionDepth--;
}

void CheckerUnit::Visit(const ReturnStmtNode& node) {
    if (functionDepth == 0) {
        ReportError("return문은 함수 내부에서만 사용할 수 있습니다.", node.token.line);
    } else if (insideInit) {
        ReportError("생성자(init)에서는 return을 사용할 수 없습니다.", node.token.line);
    }
    Traverse(node);
}

void CheckerUnit::Visit(const CallExprNode& node) {
    // 메서드 호출(children[0]이 MemberAccessExpr)은 methods가 functionScopeStack에
    // 등록되지 않아 아래 검사를 적용하면 오판된다. 존재 여부는 런타임에 검사한다.
    if (!node.children.empty() && node.children[0]->type == NodeType::MemberAccessExpr) {
        Traverse(node);
        return;
    }

    const std::string& calleeName = node.token.lexeme;

    // Robot(...)처럼 calleeName이 클래스 이름이면 인스턴스 생성 호출이다. 클래스는
    // functionScopeStack이 아니라 classScopeStack에 등록되므로 아래 함수 호출 검사를
    // 적용하면 안 된다 -- 실제로 존재하는 클래스도 항상 미정의 함수로 오판된다.
    if (LookupClass(calleeName)) {
        Traverse(node);
        return;
    }

    bool isVar = false;
    for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
        if (it->count(calleeName)) {
            isVar = true;
            break;
        }
    }

    const std::vector<std::string>* params = LookupFunction(calleeName);
    if (params) {
        if (node.children.size() != params->size()) {
            ReportError("함수 '" + calleeName + "' 호출 시 인자 개수가 일치하지 않습니다. (필요: "
                + std::to_string(params->size()) + ", 전달: " + std::to_string(node.children.size()) + ")",
                node.token.line);
        }
    } else if (isVar) {
        ReportError("'" + calleeName + "'은(는) 함수가 아니므로 호출할 수 없습니다.", node.token.line);
    } else {
        ReportError("정의되지 않은 함수 '" + calleeName + "'을(를) 호출했습니다.", node.token.line);
    }

    Traverse(node);
}

void CheckerUnit::Visit(const IdentifierNode& node) {
    // 정적 바인딩: 현재 스코프부터 바깥으로 훑어 변수가 선언된 스코프까지의
    // 거리(홉 수)를 미리 계산해둔다. 0이면 현재 스코프에 선언되어 있다는 뜻이다.
    // 못 찾으면 scopeDistance는 기본값 -1로 남고, ExecutorUnit이 기존 동적 탐색으로 폴백한다.
    const std::string& name = node.token.lexeme;
    for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
        if (it->count(name)) {
            node.scopeDistance = static_cast<int>(std::distance(scopeStack.rbegin(), it));
            return;
        }
    }
}

void CheckerUnit::Visit(const BinaryExprNode& node) {
    Traverse(node); // 하위 표현식부터 먼저 폴딩되어야 이 노드에서 상수 여부를 판단할 수 있다.

    Value_t leftValue, rightValue;
    if (!TryGetConstantValue(*node.children[0], leftValue) ||
        !TryGetConstantValue(*node.children[1], rightValue)) {
        return;
    }

    if (node.token.type == TokenType::And) {
        node.foldedValue = ToBool(leftValue) && ToBool(rightValue);
        node.isConstantFolded = true;
        return;
    }
    if (node.token.type == TokenType::Or) {
        node.foldedValue = ToBool(leftValue) || ToBool(rightValue);
        node.isConstantFolded = true;
        return;
    }
    if (node.token.type == TokenType::Eq) {
        node.foldedValue = (leftValue == rightValue);
        node.isConstantFolded = true;
        return;
    }
    if (node.token.type == TokenType::NotEq) {
        node.foldedValue = !(leftValue == rightValue);
        node.isConstantFolded = true;
        return;
    }

    // 나머지(산술/대소비교)는 숫자 상수 폴딩 범위 밖이면(문자열 등) 그대로 두고
    // 실행 단계에서 기존 로직(및 에러 처리)이 담당하게 한다.
    if (!std::holds_alternative<double>(leftValue) || !std::holds_alternative<double>(rightValue)) {
        return;
    }
    double left = std::get<double>(leftValue);
    double right = std::get<double>(rightValue);

    switch (node.token.type) {
    case TokenType::Plus:  node.foldedValue = left + right; break;
    case TokenType::Minus: node.foldedValue = left - right; break;
    case TokenType::Star:  node.foldedValue = left * right; break;
    case TokenType::Slash:
        if (right == 0.0) return; // 0으로 나누기는 런타임 에러(라인 정보 포함)로 남겨둔다.
        node.foldedValue = left / right;
        break;
    case TokenType::Percent:
        if (right == 0.0) return;
        node.foldedValue = std::fmod(left, right);
        break;
    case TokenType::Gt:   node.foldedValue = left > right; break;
    case TokenType::Lt:   node.foldedValue = left < right; break;
    case TokenType::GtEq: node.foldedValue = left >= right; break;
    case TokenType::LtEq: node.foldedValue = left <= right; break;
    default:
        return;
    }
    node.isConstantFolded = true;
}

void CheckerUnit::Visit(const UnaryExprNode& node) {
    Traverse(node);

    Value_t operandValue;
    if (!TryGetConstantValue(*node.children[0], operandValue)) {
        return;
    }

    if (node.token.type == TokenType::Not) {
        node.foldedValue = !ToBool(operandValue);
        node.isConstantFolded = true;
        return;
    }
    if (node.token.type == TokenType::Minus) {
        if (!std::holds_alternative<double>(operandValue)) return;
        node.foldedValue = -std::get<double>(operandValue);
        node.isConstantFolded = true;
    }
}

bool CheckerUnit::TryGetConstantValue(const SyntaxNode& node, Value_t& outValue) const {
    switch (node.type) {
    case NodeType::NumberLiteral:
        outValue = node.token.realValue;
        return true;
    case NodeType::BoolLiteral:
        outValue = (node.token.lexeme == "true");
        return true;
    case NodeType::BinaryExpr: {
        const auto& binNode = static_cast<const BinaryExprNode&>(node);
        if (!binNode.isConstantFolded) return false;
        outValue = binNode.foldedValue;
        return true;
    }
    case NodeType::UnaryExpr: {
        const auto& unaryNode = static_cast<const UnaryExprNode&>(node);
        if (!unaryNode.isConstantFolded) return false;
        outValue = unaryNode.foldedValue;
        return true;
    }
    default:
        // 문자열 리터럴(및 그 외 타입)은 이번 상수 폴딩 대상 범위(숫자 산술/비교/논리)에서 제외한다.
        return false;
    }
}

bool CheckerUnit::ToBool(const Value_t& value) const {
    if (std::holds_alternative<bool>(value)) return std::get<bool>(value);
    if (std::holds_alternative<double>(value)) return std::get<double>(value) != 0.0;
    return false; // 문자열/함수 값은 TryGetConstantValue에서 이미 걸러지므로 도달하지 않는다.
}

void CheckerUnit::Visit(const ArrExprNode& node) {
    Traverse(node); // 크기식 내부의 상수 폴딩(예: Arr(1 + 2))이 먼저 끝나야 판단할 수 있다.

    const SyntaxNode& sizeExpr = *node.children[0];
    if (IsObviouslyNonNumericLiteral(sizeExpr)) {
        ReportError("배열 크기는 반드시 숫자여야 합니다.", node.token.line);
    }
}

void CheckerUnit::Visit(const IndexExprNode& node) {
    Traverse(node);

    const SyntaxNode& arrayExpr = *node.children[0];
    const SyntaxNode& indexExpr = *node.children[1];

    if (IsObviouslyScalarLiteral(arrayExpr)) {
        ReportError("배열이 아닌 값에 '[]'를 사용할 수 없습니다.", node.token.line);
    }

    if (IsObviouslyNonNumericLiteral(indexExpr)) {
        ReportError("배열 인덱스는 반드시 숫자여야 합니다.", node.token.line);
    }

    // Arr(N)[i]처럼 크기와 인덱스가 모두 컴파일 타임 상수로 확정되는 경우에 한해
    // 범위를 미리 검사한다. 변수를 거치는 일반적인 경우는 값 흐름 추적이 필요해
    // 다루지 않고 실행 시점(ExecutorUnit)에 맡긴다.
    if (arrayExpr.type == NodeType::ArrExpr) {
        const auto& arrNode = static_cast<const ArrExprNode&>(arrayExpr);
        Value_t sizeValue, indexValue;
        if (TryGetConstantValue(*arrNode.children[0], sizeValue) &&
            TryGetConstantValue(indexExpr, indexValue) &&
            std::holds_alternative<double>(sizeValue) &&
            std::holds_alternative<double>(indexValue)) {
            double size = std::get<double>(sizeValue);
            double index = std::get<double>(indexValue);
            if (index < 0 || index >= size) {
                ReportError("배열 인덱스가 범위를 벗어났습니다. (크기: " + FormatNumber(size) +
                    ", 인덱스: " + FormatNumber(index) + ")", node.token.line);
            }
        }
    }
}

void CheckerUnit::Visit(const ThisExprNode& node) {
    if (classMethodDepth == 0) {
        ReportError("클래스 외부에서 'this'를 사용할 수 없습니다.", node.token.line);
    }
}

void CheckerUnit::Visit(const SuperExprNode& node) {
    if (classMethodDepth == 0) {
        ReportError("클래스 외부에서 'Super'를 사용할 수 없습니다.", node.token.line);
        return;
    }
    if (classHasParentStack.empty() || !classHasParentStack.back()) {
        ReportError("부모 클래스가 없는 클래스에서 'Super'를 사용할 수 없습니다.", node.token.line);
    }
}

void CheckerUnit::Visit(const MemberAccessExprNode& node) {
    Traverse(node);

    // 리터럴 기반 확정 오류만 잡는다: "hello".field 처럼 대상이 명백히 인스턴스가
    // 아닌 리터럴인 경우만 여기서 검사하고, 변수를 거치는 경우(var x = "hello"; x.field)는
    // 값 흐름 추적이 필요해 실행 시점(ExecutorUnit)에 맡긴다.
    const SyntaxNode& target = *node.children[0];
    if (IsObviouslyScalarLiteral(target)) {
        ReportError("인스턴스가 아닌 값에 멤버 '" + node.token.lexeme + "'로 접근할 수 없습니다.", node.token.line);
    }
}

void CheckerUnit::Visit(const ClassDeclStmtNode& node) {
    const std::string& className = node.token.lexeme;
    const std::string& parentName = node.parentNameToken.lexeme;
    bool hasParent = !parentName.empty();

    // 1. 클래스 이름 중복 선언 검사 (현재 스코프만 확인)
    if (classScopeStack.back().count(className)) {
        ReportError("클래스 '" + className + "' 중복 선언.", node.token.line);
    }

    // 2. 자기 자신 상속 검사
    if (hasParent && parentName == className) {
        ReportError("클래스 '" + className + "'은(는) 자기 자신을 상속할 수 없습니다.", node.token.line);
    } else if (hasParent) {
        // 3. 클래스가 아닌 대상/정의되지 않은 대상을 상속했는지 검사
        if (!LookupClass(parentName)) {
            bool isVar = false;
            for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
                if (it->count(parentName)) { isVar = true; break; }
            }
            if (isVar) {
                ReportError("'" + parentName + "'은(는) 클래스가 아니므로 상속할 수 없습니다.", node.token.line);
            } else {
                ReportError("정의되지 않은 클래스 '" + parentName + "'을(를) 상속했습니다.", node.token.line);
            }
        }
    }

    // 4. 검사 후 현재 스코프에 등록(자기 자신을 포함해 재귀적으로 인스턴스를 참조하는
    // 메서드 본문을 검사할 수 있도록 메서드 검사 전에 등록한다)
    classScopeStack.back()[className] = hasParent ? std::optional<std::string>(parentName) : std::nullopt;

    // 5. 메서드 이름 중복 검사
    std::unordered_set<std::string> seenMethods;
    for (const auto& methodNode : node.children) {
        const std::string& methodName = methodNode->token.lexeme;
        if (seenMethods.count(methodName)) {
            ReportError("클래스 '" + className + "'에 메서드 '" + methodName + "'이(가) 중복 선언되었습니다.", methodNode->token.line);
        }
        seenMethods.insert(methodName);
    }

    // 6. 각 메서드 본문을 this/Super/return(init) 컨텍스트를 갖춰 검사
    classHasParentStack.push_back(hasParent);
    for (const auto& methodNode : node.children) {
        CheckClassMethod(*methodNode, className);
    }
    classHasParentStack.pop_back();
}

void CheckerUnit::CheckClassMethod(const SyntaxNode& methodNode, const std::string& className) {
    const std::string& methodName = methodNode.token.lexeme;

    if (methodNode.children.empty()) {
        ReportError("클래스 '" + className + "'의 메서드 '" + methodName + "'에 본문이 없습니다.", methodNode.token.line);
        return;
    }

    // FuncDeclStmtNode와 동일한 구조: 마지막 child가 body(BlockStmt), 그 앞은 파라미터.
    // 메서드는 일반 함수 호출 네임스페이스(functionScopeStack)에는 등록하지 않는다 —
    // r.move(...)처럼 반드시 인스턴스를 통해서만 호출 가능해야 하며, 전역 함수와
    // 이름이 같아도 서로 간섭하면 안 되기 때문이다.
    const SyntaxNode* body = methodNode.children.back().get();

    std::unordered_set<std::string> seenParams;
    std::vector<std::string> paramNames;
    for (size_t i = 0; i + 1 < methodNode.children.size(); ++i) {
        const std::string& paramName = methodNode.children[i]->token.lexeme;
        if (seenParams.count(paramName)) {
            ReportError("클래스 '" + className + "'의 메서드 '" + methodName + "'의 파라미터 이름 '" + paramName + "'이(가) 중복되었습니다.", methodNode.token.line);
        }
        seenParams.insert(paramName);
        paramNames.push_back(paramName);
    }

    bool isInit = (methodName == "init");
    bool previousInsideInit = insideInit;
    insideInit = isInit;
    functionDepth++;   // return문이 유효하도록 "함수(메서드) 내부" 컨텍스트로 취급
    classMethodDepth++; // this/Super가 유효하도록 "클래스 메서드 내부" 컨텍스트로 취급

    EnterScope();
    for (const auto& paramName : paramNames) {
        scopeStack.back().insert(paramName);
    }
    body->Accept(*this);
    ExitScope();

    classMethodDepth--;
    functionDepth--;
    insideInit = previousInsideInit;
}

void CheckerUnit::Visit(const ForStmtNode& node) {
    // init/condition/update/body 전체를 "반복문 내부"로 취급한다 — import는
    // 이 구간 어디에서도 쓸 수 없다(예: for 안의 블록뿐 아니라 init에서도 금지).
    //
    // init에서 선언되는 변수(예: "var i")는 for문이 끝나면 사라져야 하므로
    // ExecutorUnit::ExecuteForStmt와 동일하게 이 노드 전체를 새 스코프 하나로
    // 감싼다. 이 프레임 하나를 빠뜨리면 ExecutorUnit의 실제 스코프 깊이와
    // 어긋나서, IdentifierNode::scopeDistance(정적으로 계산해두는 스코프까지의
    // 거리)가 잘못된 값을 가리키게 된다.
    EnterScope();
    loopDepth++;
    Traverse(node);
    loopDepth--;
    ExitScope();
}

void CheckerUnit::Visit(const ImportStmtNode& node) {
    if (loopDepth > 0) {
        ReportError("반복문 내부에서는 import를 사용할 수 없습니다.", node.token.line);
    }

    const std::string& path = node.children[0]->token.lexeme;
    const std::string& alias = node.children[1]->token.lexeme;

    // 같은 scope 내 중복 import / 상위(조상) scope에서 이미 import된 파일 재-import 금지.
    // 현재 스코프부터 바깥으로 훑으므로, 형제 스코프의 독립적인 import는 서로 다른
    // 스택 프레임에 있어 자연히 허용되고, 조상-자손 관계일 때만 걸린다.
    for (auto it = importScopeStack.rbegin(); it != importScopeStack.rend(); ++it) {
        if (it->count(path)) {
            ReportError("파일 '" + path + "'은(는) 이미 import되었습니다.", node.token.line);
            break;
        }
    }
    importScopeStack.back().insert(path);

    // alias 이름 충돌: 변수 중복 선언 검사와 동일한 메커니즘(현재 스코프만 확인)을 재사용.
    if (scopeStack.back().count(alias)) {
        ReportError("별칭 '" + alias + "' 중복 선언.", node.token.line);
    }
    scopeStack.back().insert(alias);

    // node.children[2:]는 import 대상 파일의 최상위 선언들(ExecutorUnit이 모듈 멤버로
    // 등록하는 것과 동일한 목록)이다. 이 파일만의 독립된 스코프에서 검사해야
    // alias.member로만 접근 가능하고 바깥 스코프 이름과 충돌하지 않는다.
    EnterScope();
    for (size_t i = 2; i < node.children.size(); ++i) {
        node.children[i]->Accept(*this);
    }
    ExitScope();
}

bool CheckerUnit::IsObviouslyScalarLiteral(const SyntaxNode& node) const {
    return node.type == NodeType::NumberLiteral ||
        node.type == NodeType::StringLiteral ||
        node.type == NodeType::BoolLiteral;
}

bool CheckerUnit::IsObviouslyNonNumericLiteral(const SyntaxNode& node) const {
    return node.type == NodeType::StringLiteral || node.type == NodeType::BoolLiteral;
}

std::string CheckerUnit::FormatNumber(double value) const {
    if (value == std::floor(value)) {
        return std::to_string(static_cast<long long>(value));
    }
    return std::to_string(value);
}

bool CheckerUnit::IsReferencingVar(const SyntaxNode* node, const std::string& varName) {
    if (!node) return false;

    // Identifier 타입이고 이름이 같다면 자기 참조
    if (node->type == NodeType::Identifier && node->token.lexeme == varName) {
        return true;
    }

    // 하위 트리 재귀 탐색
    for (auto& child : node->children) {
        if (IsReferencingVar(child.get(), varName)) return true;
    }
    return false;
}

const std::vector<std::string>* CheckerUnit::LookupFunction(const std::string& name) const {
    for (auto it = functionScopeStack.rbegin(); it != functionScopeStack.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

const std::optional<std::string>* CheckerUnit::LookupClass(const std::string& name) const {
    for (auto it = classScopeStack.rbegin(); it != classScopeStack.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

void CheckerUnit::ReportError(const std::string& message, int line) {
    std::cerr << "오류 [Line " << line << "]: " << message << std::endl;
    hasError = true;
}
