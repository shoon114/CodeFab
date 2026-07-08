#include "CheckerUnit.h"
#include <cmath>
#include <iostream>
#include <iterator>

bool CheckerUnit::Check(SyntaxNode* root) {
    if (!root) return true;
    scopeStack.clear();
    functionScopeStack.clear();
    hasError = false;
    functionDepth = 0;
    EnterScope(); // Global 스코프
    root->Accept(*this);
    ExitScope();
    return !hasError;
}

void CheckerUnit::Visit(const VarDeclareStatementNode& node) {
    std::string varName = node.token.lexeme;

    // 1. 중복 선언 검사 (현재 스코프만 확인)
    if (scopeStack.back().count(varName)) {
        ReportError("변수 '" + varName + "' 중복 선언.", node.token.line);
    }

    // 2. 자기 참조 검사: 초기화식 노드(보통 자식 노드에 위치) 탐색
    // var a = a + 1; 구조에서 우항이 첫 번째 자식이라고 가정
    if (!node.children.empty()) {
        if (IsReferencingVar(node.children[0].get(), varName)) {
            ReportError("자신의 초기화식에서 변수 '" + varName + "'을(를) 참조할 수 없습니다.", node.token.line);
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
    }
    Traverse(node);
}

void CheckerUnit::Visit(const CallExprNode& node) {
    const std::string& calleeName = node.token.lexeme;

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

bool CheckerUnit::IsReferencingVar(SyntaxNode* node, const std::string& varName) {
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

void CheckerUnit::ReportError(const std::string& message, int line) {
    std::cerr << "오류 [Line " << line << "]: " << message << std::endl;
    hasError = true;
}
