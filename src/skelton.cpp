공통 값/에러 타입

// Token.h
#pragma once
#include <string>
#include <vector>

enum class TokenType { Identifier, Number, Operator, Keyword, EndOfFile /* ... */ };

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
};

using TokenList = std::vector<Token>;

// CodeFabException.h
#pragma once
#include <stdexcept>
#include <string>

class CodeFabException : public std::runtime_error {
public:
    CodeFabException(std::string message, int line, int column)
        : std::runtime_error(std::move(message)), line(line), column(column) {}
    int line;
    int column;
};

class TokenizeError  : public CodeFabException { using CodeFabException::CodeFabException; };
class ParseError     : public CodeFabException { using CodeFabException::CodeFabException; };
class CheckError     : public CodeFabException { using CodeFabException::CodeFabException; };
class ExecuteError   : public CodeFabException { using CodeFabException::CodeFabException; };
SyntaxTree / Visitor 인터페이스

// SyntaxTree.h
#pragma once

class Visitor;

class SyntaxTree {
public:
    virtual ~SyntaxTree() = default;
    virtual void Accept(Visitor& visitor) = 0;
};

// Visitor.h
#pragma once

class SyntaxTree;
// 실제로는 노드 종류별(BinaryExprNode, AssignNode ...) 오버로드가 필요하지만
// 다이어그램 수준에서는 아래처럼 최소 계약만 정의
class Visitor {
public:
    virtual ~Visitor() = default;
    virtual void Visit(SyntaxTree& node) = 0;
};
AssemblerUnit — 계약

// AssemblerUnit.h
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
CheckerUnit — 계약

// CheckerUnit.h
#pragma once
#include "SyntaxTree.h"

class CheckerUnit {
public:
    virtual ~CheckerUnit() = default;

    // 계약:
    //  - tree는 null이 아니어야 한다 (호출부 책임).
    //  - 타입/스코프/의미 오류가 있으면 CheckError(line, column)를 던진다.
    //  - 통과 시 부작용 없음 (tree를 변형하지 않음).
    virtual void Check(SyntaxTree& tree) = 0;
};
ExecutorUnit — 계약

// ExecutorUnit.h
#pragma once
#include "SyntaxTree.h"

class ExecutorUnit {
public:
    virtual ~ExecutorUnit() = default;

    // 계약:
    //  - tree는 CheckerUnit::Check()를 통과한 트리여야 한다 (사전조건).
    //  - 실행 중 런타임 오류가 있으면 ExecuteError(line, column)를 던진다.
    virtual void Execute(SyntaxTree& tree) = 0;
};
CodeFabInterpreter — 오케스트레이션 계약

// CodeFabInterpreter.h
#pragma once
#include <memory>
#include <string>
#include "AssemblerUnit.h"
#include "CheckerUnit.h"
#include "ExecutorUnit.h"

class CodeFabInterpreter {
public:
    CodeFabInterpreter(std::unique_ptr<AssemblerUnit> assembler,
                        std::unique_ptr<CheckerUnit> checker,
                        std::unique_ptr<ExecutorUnit> executor)
        : assembler(std::move(assembler)),
          checker(std::move(checker)),
          executor(std::move(executor)) {}

    // 파이프라인: Tokenize -> Parse -> Check -> Execute
    // 각 단계에서 발생한 예외(TokenizeError/ParseError/CheckError/ExecuteError)는
    // 상위로 그대로 전파한다 (여기서 삼키지 않음).
    void Run(const std::string& sourceCode) {
        TokenList tokens = assembler->Tokenize(sourceCode);
        std::unique_ptr<SyntaxTree> tree = assembler->Parse(tokens);
        checker->Check(*tree);
        executor->Execute(*tree);
    }

private:
    std::unique_ptr<AssemblerUnit> assembler;
    std::unique_ptr<CheckerUnit> checker;
    std::unique_ptr<ExecutorUnit> executor;
};
모듈 간 데이터 흐름 요약
호출	입력	출력	실패 시
Assembler.Tokenize	sourceCode: string	TokenList	TokenizeError
Assembler.Parse	TokenList	unique_ptr<SyntaxTree>	ParseError
Checker.Check	SyntaxTree& (수정 없음)	void	CheckError
Executor.Execute	SyntaxTree& (Check 통과 전제)	void (부작용: 실행 결과 출력/상태 변경)	ExecuteError