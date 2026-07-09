#pragma once
#include "Token.h"
#include <string>
#include <vector>

// FileMode/DebugMode가 공통으로 쓰는 파일 로딩 + 토큰화 유틸리티.
class SourceFileLoader {
public:
	// 파일 전체를 outContent에 읽어 담는다. 파일을 열 수 없으면 false를 반환한다.
	static bool LoadFile(const std::string& path, std::string& outContent);
	// 디버그 모드가 현재 줄을 화면에 표시할 때 쓴다.
	static std::vector<std::string> SplitIntoLines(const std::string& content);
	// Tokenizer::GetCodeFromUser()는 std::cin을 rdbuf()로 통째로 읽으므로, 이미
	// 문자열로 가진 소스를 토큰화할 때도 std::cin을 잠시 바꿔치기해서 재사용한다.
	static TokenList Tokenize(const std::string& source);
};
