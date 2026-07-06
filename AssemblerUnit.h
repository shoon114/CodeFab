#pragma once
#include <memory>
#include <string>
#include "Token.h"
#include "SyntaxTree.h"

class AssemblerUnit {
public:
    virtual ~AssemblerUnit() = default;

    // 계약:
    //  - source가 빈 문자열이면 EndOfFile 토큰 하나만 담긴 리스트를 반환한다.
    //  - 어휘 규칙에 어긋나면 TokenizeError(line, column)를 던진다.
    virtual TokenList Tokenize(const std::string& source) = 0;

    // 계약:
    //  - tokens는 Tokenize()가 만든 결과여야 하며 마지막이 EndOfFile이어야 한다.
    //  - 문법 규칙에 어긋나면 ParseError(line, column)를 던진다.
    //  - 성공 시 소유권이 이전되는 SyntaxTree 루트 노드를 반환한다.
    virtual std::unique_ptr<SyntaxTree> Parse(const TokenList& tokens) = 0;
};
