#include "CheckerUnit.h"
#include <iostream>

bool CheckerUnit::Check(SyntaxNode* root) {
    if (!root) return true;
    scopeStack.clear();
    EnterScope(); // Global 스코프
    Visit(root);
    ExitScope();
    return true;
}

void CheckerUnit::Visit(SyntaxNode* node) {
    if (!node) return;

    // 블록 진입/탈출 시 스코프 관리
    bool isBlock = (node->type == NodeType::BlockStmt);
    if (isBlock) EnterScope();

    if (node->type == NodeType::VarDeclareStatement) {
        CheckVarDecl(node);
    }

    // 자식 노드 재귀 방문
    for (auto& child : node->children) {
        Visit(child.get());
    }

    if (isBlock) ExitScope();
}

void CheckerUnit::CheckVarDecl(SyntaxNode* node) {
    std::string varName = node->token.lexeme;

    // 1. 중복 선언 검사 (현재 스코프만 확인)
    if (scopeStack.back().count(varName)) {
        std::cerr << "오류 [Line " << node->token.line << "]: 변수 '" << varName << "' 중복 선언." << std::endl;
    }

    // 2. 자기 참조 검사: 초기화식 노드(보통 자식 노드에 위치) 탐색
    // var a = a + 1; 구조에서 우항이 첫 번째 자식이라고 가정
    if (!node->children.empty()) {
        if (IsReferencingVar(node->children[0].get(), varName)) {
            std::cerr << "오류 [Line " << node->token.line << "]: 자신의 초기화식에서 변수 '"
                << varName << "'을(를) 참조할 수 없습니다." << std::endl;
        }
    }

    // 3. 검사 후 현재 스코프에 등록
    scopeStack.back().insert(varName);
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