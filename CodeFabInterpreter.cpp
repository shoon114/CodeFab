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
		bool sawOpenBrace = false;
		for (const Token& token : tokenList) {
			if (token.type == TokenType::LBrace) {
				braceDepth++;
				sawOpenBrace = true;
			} else if (token.type == TokenType::RBrace) {
				braceDepth--;
			}
		}
		if (braceDepth > 0) {
			continue; // 블록이 아직 안 닫혔으니 다음 줄을 더 받는다
		}

		// "if (true)"처럼 '{'가 아직 한 번도 나오지 않은 채로 끝난 줄은 브레이스
		// 개수만 보면 0(균형)이라 "완성된 문장"으로 착각하기 쉽지만, 실제로는
		// 다음 줄에 올 '{'를 기다리는 중인 미완성 상태다. 이 상태에서 그대로
		// 파싱을 시도하면 예외가 나서 버퍼가 통째로 비워지고, 뒤이어 입력되는
		// '{ ... }' 블록이 앞의 if/for 조건과 분리된 채 무조건 실행되어 버린다.
		// '{'가 한 번도 안 나왔다면, 문장이 ';'로 완전히 끝난 경우에만 실행하고
		// 그렇지 않으면 계속 버퍼링한다.
		bool endsWithSemicolon = tokenList.size() >= 2
			&& tokenList[tokenList.size() - 2].type == TokenType::Semicolon;
		if (!sawOpenBrace && !endsWithSemicolon) {
			continue;
		}

		try {
			std::unique_ptr<SyntaxNode> tree = assembler.Parse(tokenList);
			checker.Check(tree.get());
			executor.Execute(*tree);
			std::cout << std::endl;
		} catch (const std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
		buffer.clear();
	}

	return 0;
#endif
}