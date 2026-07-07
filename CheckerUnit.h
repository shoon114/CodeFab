#pragma once
#include "SyntaxNode.h"
#include <unordered_set>
#include <vector>
#include <string>

class CheckerUnit {
public:
    // 전체 트리 검사 실행
    bool Check(SyntaxNode* root);

private:
    // 스코프 스택: 각 블록마다 선언된 변수명 저장
    std::vector<std::unordered_set<std::string>> scopeStack;

    void EnterScope() { scopeStack.push_back({}); }
    void ExitScope() { scopeStack.pop_back(); }

    void Visit(SyntaxNode* node);
    void CheckVarDecl(SyntaxNode* node);
    bool IsReferencingVar(SyntaxNode* node, const std::string& varName);
};