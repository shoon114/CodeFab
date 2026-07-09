#pragma once
#include "ShellMode.h"
#include <string>

// 파일 모드: .txt 소스 파일 전체를 한 번에 읽어 실행한다.
class FileMode : public ShellMode {
public:
	explicit FileMode(std::string path);
	int Run() override;

private:
	std::string path;
};
