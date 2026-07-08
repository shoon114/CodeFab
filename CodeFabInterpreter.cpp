#ifdef _DEBUG
#include "gmock/gmock.h"
using namespace testing;
#else
#include <iostream>
#include <sstream>
#include <string>
#include "Tokenizer.h"
#include "AssemblerUnit.h"
#include "CheckerUnit.h"
#include "ExecutorUnit.h"
#endif

int main() {
#ifdef _DEBUG
	InitGoogleMock();
	return RUN_ALL_TESTS();
#else
	AssemblerUnit assembler;
	CheckerUnit checker;
	ExecutorUnit executor;

	// 줄 단위로 입력을 누적하다가, 누적된 버퍼를 토큰화했을 때 '{'/'}' 개수가
	// 맞아떨어지는 시점(블록이 전부 닫힌 시점)에만 그 버퍼 전체를 토큰화/파싱/실행한다.
	// 이렇게 하지 않으면 블록이 여러 줄에 걸친 입력(if/for 등)이 첫 줄만으로
	// 파싱을 시도하다가 '}'를 못 찾고 실패한다.
	// Tokenizer::GetCodeFromUser()는 std::cin을 rdbuf()로 통째로 읽으므로,
	// 매번 std::cin을 누적 버퍼만 담은 istringstream으로 잠시 바꿔치기해서 호출한다.
	// 함수 선언(FuncDeclStmt)은 FunctionObject::body에 body 노드를 raw pointer로만
	// 보관한다. 그 노드가 속한 tree가 사라지면 이후 줄에서 함수를 호출할 때 dangling
	// pointer가 되므로, 파싱된 tree는 매 줄이 끝나도 버리지 않고 세션이 끝날 때까지
	// 계속 붙잡아둔다(변수처럼 executor.scopes도 REPL 전체에 걸쳐 유지되는 것과 동일한 이유).
	std::vector<std::unique_ptr<SyntaxNode>> trees;
	std::string buffer;
	std::string line;
	while (std::getline(std::cin, line)) {
		if (buffer.empty() && line.empty()) {
			continue;
		}
		buffer += line + "\n";

		Tokenizer tokenizer;
		std::istringstream bufferInput(buffer);
		std::streambuf* originalCinBuffer = std::cin.rdbuf(bufferInput.rdbuf());
		tokenizer.GetCodeFromUser();
		std::cin.rdbuf(originalCinBuffer);

		TokenList tokenList = tokenizer.CreateTokenForCode();

		int braceDepth = 0;
		for (const Token& token : tokenList) {
			if (token.type == TokenType::LBrace) {
				braceDepth++;
			} else if (token.type == TokenType::RBrace) {
				braceDepth--;
			}
		}
		if (braceDepth > 0) {
			continue; // 블록이 아직 안 닫혔으니 다음 줄을 더 받는다
		}

		try {
			trees.push_back(assembler.Parse(tokenList));
			checker.Check(trees.back().get());
			executor.Execute(*trees.back());
			std::cout << std::endl;
		} catch (const std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
		buffer.clear();
	}

	return 0;
#endif
}