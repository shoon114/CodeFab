#ifdef _DEBUG
#include "gmock/gmock.h"
using namespace testing;
#else
#include <iostream>
#include "Tokenizer.h"
#include "AssemblerUnit.h"
#include "ExecutorUnit.h"
#endif

int main() {
#ifdef _DEBUG
	InitGoogleMock();
	return RUN_ALL_TESTS();
#else
	Tokenizer tokenizer;
	tokenizer.GetCodeFromUser();
	TokenList tokenList = tokenizer.CreateTokenForCode();

	AssemblerUnit assembler;
	ExecutorUnit executor;
	try {
		std::unique_ptr<SyntaxNode> tree = assembler.Parse(tokenList);
		executor.Execute(*tree);
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
#endif
}