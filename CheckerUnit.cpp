#include "CheckerUnit.h"
#include <iostream>

bool CheckerUnit::Check(SyntaxNode* root) {
    if (!root) return true;
    scopeStack.clear();
    functionScopeStack.clear();
    hasError = false;
    functionDepth = 0;
    EnterScope(); // Global 스코프
    Visit(root);
    ExitScope();
    return !hasError;
}

void CheckerUnit::Visit(SyntaxNode* node) {
    if (!node) return;

    if (node->type == NodeType::FuncDeclStmt) {
        CheckFuncDecl(node);
        return;
    }

    if (node->type == NodeType::ReturnStmt) {
        CheckReturnStmt(node);
    }

    if (node->type == NodeType::CallExpr) {
        CheckCallExpr(node);
    }

    bool isBlock = (node->type == NodeType::BlockStmt);
    if (isBlock) EnterScope();

    if (node->type == NodeType::VarDeclareStatement) {
        CheckVarDecl(node);
    }

    for (auto& child : node->children) {
        Visit(child.get());
    }

    if (isBlock) ExitScope();
}

void CheckerUnit::CheckVarDecl(SyntaxNode* node) {
    std::string varName = node->token.lexeme;

    // 1. 중복 선언 검사 (현재 스코프만 확인)
    if (scopeStack.back().count(varName)) {
        ReportError("변수 '" + varName + "' 중복 선언.", node->token.line);
    }

    // 2. 자기 참조 검사: 초기화식 노드(보통 자식 노드에 위치) 탐색
    // var a = a + 1; 구조에서 우항이 첫 번째 자식이라고 가정
    if (!node->children.empty()) {
        if (IsReferencingVar(node->children[0].get(), varName)) {
            ReportError("자신의 초기화식에서 변수 '" + varName + "'을(를) 참조할 수 없습니다.", node->token.line);
        }
    }

    // 3. 검사 후 현재 스코프에 등록
    scopeStack.back().insert(varName);
}

void CheckerUnit::CheckFuncDecl(SyntaxNode* node) {
    const std::string& funcName = node->token.lexeme;

    if (node->children.empty()) {
        ReportError("함수 '" + funcName + "'에 본문이 없습니다.", node->token.line);
        return;
    }

    // 마지막 child가 body(BlockStmt), 그 앞은 파라미터(Identifier) 목록이라고 가정
    SyntaxNode* body = node->children.back().get();

    std::vector<std::string> paramNames;
    std::unordered_set<std::string> seenParams;
    for (size_t i = 0; i + 1 < node->children.size(); ++i) {
        const std::string& paramName = node->children[i]->token.lexeme;
        if (seenParams.count(paramName)) {
            ReportError("함수 '" + funcName + "'의 파라미터 이름 '" + paramName + "'이(가) 중복되었습니다.", node->token.line);
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
    Visit(body);
    ExitScope();
    functionDepth--;
}

void CheckerUnit::CheckReturnStmt(SyntaxNode* node) {
    if (functionDepth == 0) {
        ReportError("return문은 함수 내부에서만 사용할 수 있습니다.", node->token.line);
    }
}

void CheckerUnit::CheckCallExpr(SyntaxNode* node) {
    const std::string& calleeName = node->token.lexeme;

    bool isVar = false;
    for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
        if (it->count(calleeName)) {
            isVar = true;
            break;
        }
    }

    const std::vector<std::string>* params = LookupFunction(calleeName);
    if (params) {
        if (node->children.size() != params->size()) {
            ReportError("함수 '" + calleeName + "' 호출 시 인자 개수가 일치하지 않습니다. (필요: "
                + std::to_string(params->size()) + ", 전달: " + std::to_string(node->children.size()) + ")",
                node->token.line);
        }
    } else if (isVar) {
        ReportError("'" + calleeName + "'은(는) 함수가 아니므로 호출할 수 없습니다.", node->token.line);
    } else {
        ReportError("정의되지 않은 함수 '" + calleeName + "'을(를) 호출했습니다.", node->token.line);
    }
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