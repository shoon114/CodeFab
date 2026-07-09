#pragma once
#include "NodeVisitor.h"
#include "SyntaxNode.h"
#include <optional>
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
    void Visit(const ThisExprNode& node) override;
    void Visit(const SuperExprNode& node) override;
    void Visit(const MemberAccessExprNode& node) override;
    void Visit(const ClassDeclStmtNode& node) override;
    void Visit(const ForStmtNode& node) override;
    void Visit(const ImportStmtNode& node) override;

private:
    // 스코프 스택: 각 블록마다 선언된 변수명 저장
    std::vector<std::unordered_set<std::string>> scopeStack;
    std::vector<std::unordered_map<std::string, std::vector<std::string>>> functionScopeStack;
    // 스코프별로 선언된 클래스 이름 -> 부모 클래스 이름(없으면 nullopt).
    std::vector<std::unordered_map<std::string, std::optional<std::string>>> classScopeStack;
    // 스코프별로 이미 import한 파일 경로 집합. 현재 스코프부터 바깥(조상)까지 훑어
    // 같은 경로가 있으면 중복 import로 판단한다 — 형제 스코프의 독립적인 import는
    // 서로 다른 스택 프레임에 있으므로 자연히 허용된다.
    std::vector<std::unordered_set<std::string>> importScopeStack;
    bool hasError = false;
    int functionDepth = 0;
    // 현재 클래스 메서드(생성자 init 포함) 본문 내부인지 추적 — this/Super 위치 제약에 사용.
    int classMethodDepth = 0;
    // 현재 init 메서드 본문 내부인지 — init에서의 return 사용 금지에 사용.
    bool insideInit = false;
    // 방문 중인 클래스들이 부모를 가지고 있는지 여부의 스택(중첩 방어용).
    std::vector<bool> classHasParentStack;
    // 현재 for문 내부인지 추적 — 반복문 내부에서의 import 사용 금지에 사용.
    int loopDepth = 0;

    void EnterScope() {
        scopeStack.push_back({});
        functionScopeStack.push_back({});
        classScopeStack.push_back({});
        importScopeStack.push_back({});
    }
    void ExitScope() {
        scopeStack.pop_back();
        functionScopeStack.pop_back();
        classScopeStack.pop_back();
        importScopeStack.pop_back();
    }

    bool IsReferencingVar(const SyntaxNode* node, const std::string& varName);
    const std::vector<std::string>* LookupFunction(const std::string& name) const;
    const std::optional<std::string>* LookupClass(const std::string& name) const;
    void ReportError(const std::string& message, int line);
    // 클래스 메서드 하나(파라미터 중복 검사 + this/Super/return 컨텍스트를 갖춘 본문 검사)를 처리한다.
    void CheckClassMethod(const SyntaxNode& methodNode, const std::string& className);

    // 상수 폴딩(Constant Folding): node가 리터럴이거나 이미 폴딩된 상수식이면
    // 그 값을 outValue에 담아 true를 반환한다.
    bool TryGetConstantValue(const SyntaxNode& node, Value_t& outValue) const;
    bool ToBool(const Value_t& value) const;

    // 배열(Arr/Index) 관련 리터럴 기반 정적 검사 헬퍼.
    // 값 흐름(변수에 배열이 대입됐는지 등)은 추적하지 않고, 리터럴만으로 100% 확정
    // 가능한 오류만 잡는다(나머지는 ExecutorUnit이 실행 시점에 처리).
    // 배열이 아닌/스칼라 리터럴인지 판별(NumberLiteral/StringLiteral/BoolLiteral).
    // 배열 인덱싱 대상 검사와 인스턴스 멤버 접근 대상 검사에서 공용으로 쓰인다.
    bool IsObviouslyScalarLiteral(const SyntaxNode& node) const;
    bool IsObviouslyNonNumericLiteral(const SyntaxNode& node) const;
    std::string FormatNumber(double value) const;
};
