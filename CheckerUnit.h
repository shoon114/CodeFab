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
    void Visit(const IdentifierNode& node) override;
    void Visit(const BinaryExprNode& node) override;
    void Visit(const UnaryExprNode& node) override;
    void Visit(const ArrExprNode& node) override;
    void Visit(const IndexExprNode& node) override;

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

    // 상수 폴딩(Constant Folding): node가 리터럴이거나 이미 폴딩된 상수식이면
    // 그 값을 outValue에 담아 true를 반환한다.
    bool TryGetConstantValue(const SyntaxNode& node, Value_t& outValue) const;
    bool ToBool(const Value_t& value) const;

    // 배열(Arr/Index) 관련 리터럴 기반 정적 검사 헬퍼.
    // 값 흐름(변수에 배열이 대입됐는지 등)은 추적하지 않고, 리터럴만으로 100% 확정
    // 가능한 오류만 잡는다(나머지는 ExecutorUnit이 실행 시점에 처리).
    bool IsObviouslyNotArray(const SyntaxNode& node) const;
    bool IsObviouslyNonNumericLiteral(const SyntaxNode& node) const;
    std::string FormatNumber(double value) const;
};
