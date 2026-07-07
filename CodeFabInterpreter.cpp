#ifdef _DEBUG
#include "gmock/gmock.h"
using namespace testing;
#else
#include <iostream>
#include <sstream>
#include <string>
#include "Tokenizer.h"
#include "AssemblerUnit.h"
#include "ExecutorUnit.h"
#endif

int main() {
#ifdef _DEBUG
	InitGoogleMock();
	return RUN_ALL_TESTS();
#else
	AssemblerUnit assembler;
	ExecutorUnit executor;

	// 한 줄 입력 -> Enter 시 그 줄까지 바로 토큰화/파싱/실행해 결과를 출력한다.
	// Tokenizer::GetCodeFromUser()는 std::cin을 rdbuf()로 통째로 읽으므로,
	// 매 줄마다 std::cin을 그 줄만 담은 istringstream으로 잠시 바꿔치기해서 호출한다.
	std::string line;
	while (std::getline(std::cin, line)) {
		if (line.empty()) {
			continue;
		}

		Tokenizer tokenizer;
		std::istringstream lineInput(line);
		std::streambuf* originalCinBuffer = std::cin.rdbuf(lineInput.rdbuf());
		tokenizer.GetCodeFromUser();
		std::cin.rdbuf(originalCinBuffer);

		TokenList tokenList = tokenizer.CreateTokenForCode();

		try {
			std::unique_ptr<SyntaxNode> tree = assembler.Parse(tokenList);
			executor.Execute(*tree);
			std::cout << std::endl;
		} catch (const std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
	}

	return 0;
#endif
}