#ifdef _DEBUG
#include "gmock/gmock.h"
using namespace testing;
#else
#include <iostream>
#include <memory>
#include <string>
#include "ShellMode.h"
#include "PromptMode.h"
#include "FileMode.h"
#include "DebugMode.h"

namespace {
	// argv로 어떤 ShellMode를 만들지만 결정한다. 결정된 뒤로는 main()도
	// 각 모드의 내부 구현(파이프라인/디버그 상태 등)을 전혀 모른다.
	std::unique_ptr<ShellMode> SelectMode(int argc, char* argv[]) {
		if (argc == 1) {
			return std::make_unique<PromptMode>();
		}

		std::string modeName = argv[1];
		if (modeName == "run" && argc >= 3) {
			return std::make_unique<FileMode>(argv[2]);
		}
		if (modeName == "debug" && argc >= 3) {
			return std::make_unique<DebugMode>(argv[2]);
		}
		return nullptr;
	}
}
#endif

int main(int argc, char* argv[]) {
#ifdef _DEBUG
	InitGoogleMock();
	return RUN_ALL_TESTS();
#else
	std::unique_ptr<ShellMode> mode = SelectMode(argc, argv);
	if (!mode) {
		std::cerr << "Usage: factory [run <file>|debug <file>]" << std::endl;
		return 1;
	}
	return mode->Run();
#endif
}
