#pragma once
#include "NodeVisitor.h"
#include "SyntaxNode.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

class CheckerUnit : public NodeVisitor {
public:
    // 전체 트리 검사 실행
    bool Check(SyntaxNode* root);

    void Visit(const VarDeclareStatementNode& node) override;
    void Visit(const BlockStmtNode& node) override;
    void Visit(const FuncDeclStmtNode& node) override;
    void Visit(const ReturnStmtNode& node) override;
    void Visit(const CallExprNode& node) override;

private:
    // 스코프 스택: 각 블록마다 선언된 변수명 저장
    std::vector<std::unordered_set<std::string>> scopeStack;
    std::vector<std::unordered_map<std::string, std::vector<std::string>>> functionScopeStack;
    bool hasError = false;
    int functionDepth = 0;

    void EnterScope() { scopeStack.push_back({}); functionScopeStack.push_back({}); }
    void ExitScope() { scopeStack.pop_back(); functionScopeStack.pop_back(); }

    bool IsReferencingVar(SyntaxNode* node, const std::string& varName);
    const std::vector<std::string>* LookupFunction(const std::string& name) const;
    void ReportError(const std::string& message, int line);
};
