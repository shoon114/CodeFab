#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "SyntaxNode.h"

// Checker 테스트에서 SyntaxNode 트리를 손으로 조립하기 위한 헬퍼.
// NodeType별 token/children 의미는 docs/SyntaxNode-Contract.md 참고.
// Assembler Unit이 완성되면 이 헬퍼로 만든 트리 대신 실제
// AssemblerUnit::Tokenize()/Parse() 결과로 테스트를 옮겨도 된다
// (이 헬퍼들은 Checker 단독 회귀 테스트용으로 계속 남겨둬도 무방).
namespace testutil {

using NodePtr = std::unique_ptr<SyntaxNode>;

inline Token makeToken(TokenType type, const std::string& lexeme, int line, int column = 1) {
    return Token{type, lexeme, line, column};
}

inline NodePtr makeNode(NodeType type, Token token, std::vector<NodePtr> children = {}) {
    auto node = std::make_unique<SyntaxNode>();
    node->type = type;
    node->token = std::move(token);
    node->children = std::move(children);
    return node;
}

inline Token identifierToken(const std::string& lexeme, int line, int column = 1) {
    return makeToken(TokenType::Identifier, lexeme, line, column);
}

// 숫자 리터럴 노드
inline NodePtr numberLiteral(const std::string& lexeme, int line) {
    return makeNode(NodeType::NumberLiteral, makeToken(TokenType::Number, lexeme, line));
}

// 변수 참조 노드 (표현식으로 쓰이는 Identifier)
inline NodePtr identifier(const std::string& name, int line) {
    return makeNode(NodeType::Identifier, identifierToken(name, line));
}

// var name = initializer;  (initializer가 nullptr이면 초기화 없는 선언)
inline NodePtr varDecl(const std::string& name, int line, NodePtr initializer = nullptr, int column = 1) {
    std::vector<NodePtr> children;
    if (initializer) children.push_back(std::move(initializer));
    return makeNode(NodeType::VarDeclStmt, identifierToken(name, line, column), std::move(children));
}

// left op right
inline NodePtr binary(NodePtr left, TokenType opType, const std::string& opLexeme, int line, NodePtr right) {
    std::vector<NodePtr> children;
    children.push_back(std::move(left));
    children.push_back(std::move(right));
    return makeNode(NodeType::BinaryExpr, makeToken(opType, opLexeme, line), std::move(children));
}

// { statements... } — 자체 로컬 스코프를 갖는 블록
inline NodePtr block(std::vector<NodePtr> statements) {
    return makeNode(NodeType::BlockStmt, Token{}, std::move(statements));
}

// AssemblerUnit::Parse()가 반환할 최상위 루트 (Global, 스코프 push 없음)
inline NodePtr program(std::vector<NodePtr> statements) {
    return makeNode(NodeType::Program, Token{}, std::move(statements));
}

// std::vector<NodePtr>{a, b, c}는 unique_ptr라 initializer_list로 못 묶으므로,
// 가변 인자로 편하게 vector를 만들기 위한 헬퍼.
template <typename... Nodes>
std::vector<NodePtr> nodes(Nodes... ns) {
    std::vector<NodePtr> result;
    (result.push_back(std::move(ns)), ...);
    return result;
}

} // namespace testutil
