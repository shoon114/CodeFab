#pragma once

// 공장 제어 쉘의 실행 모드(프롬프트/파일/디버그) 공통 인터페이스.
// main()은 argv로 어떤 ShellMode를 만들지만 결정하고, 이후로는 Run()만 호출한다.
// 각 구현체는 자신만의 파이프라인(AssemblerUnit/CheckerUnit/ExecutorUnit)을
// Run() 안에서 지역으로 소유하므로 모드 간에 상태를 공유하지 않는다.
class ShellMode {
public:
	virtual ~ShellMode() = default;
	virtual int Run() = 0;
};
