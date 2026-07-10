#pragma once
#include <string>
#include <vector>

class ExecutorUnit;

// 디버그 모드에서 "정지 시점마다 뭘 보여줄지"만 담당한다. "언제 정지할지"는
// 전혀 모른다 -- 그건 StepController의 책임이다.
//
// Watches는 등록된 각 이름을 executor.TryGetVariable/CurrentScope로 조회해
// "[LOCAL]"/"[GLOBAL]" 스코프 표시와 함께 값+타입을 출력한다("d[0]"처럼 대괄호
// 인덱스가 붙은 이름은 배열 변수를 찾아 그 원소값만 가리킨다). 존재하지 않는
// 이름/범위 밖 인덱스/배열이 아닌 값에 대한 인덱스는 "<undefined>"로 표시한다.
// Inspect는 watch 목록과 무관하게 executor.CurrentScope()의 변수 전부를 같은
// 값+타입 포맷으로 출력한다.
// 실제 화면 출력(std::cout)은 이 클래스가 하지 않는다 -- 포맷된 문자열만 만들어
// 반환하고, 그걸 언제/어디에 찍을지는 호출자(DebugSession)의 책임이다.
class WatchList {
public:
	void Add(const std::string& name);
	void Remove(const std::string& name);
	bool Contains(const std::string& name) const;
	std::string Watches(const ExecutorUnit& executor) const;
	std::string Inspect(const ExecutorUnit& executor) const;

private:
	std::vector<std::string> names; // 추가한 순서 유지
};
