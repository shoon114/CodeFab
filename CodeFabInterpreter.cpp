#include <windows.h>

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
	// 소스/실행 문자셋을 /utf-8로 빌드해도, 콘솔(conhost)의 출력 코드페이지가
	// 한국어 Windows 기본값(CP949)이면 UTF-8 바이트가 한글로 안 깨지고 깨져
	// 보인다. 실행 시작 시점에 콘솔 코드페이지 자체를 UTF-8로 맞춰준다.
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

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
