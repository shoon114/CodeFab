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
class WatchList {
public:
	void Add(const std::string& name);
	void Remove(const std::string& name);
	bool Contains(const std::string& name) const;
	void Watches(const ExecutorUnit& executor) const;

private:
	std::vector<std::string> names; // 추가한 순서 유지
};
