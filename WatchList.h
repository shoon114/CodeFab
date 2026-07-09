#pragma once
#include <string>
#include <vector>

class ExecutorUnit;

// 디버그 모드에서 "정지 시점마다 뭘 보여줄지"만 담당한다. "언제 정지할지"는
// 전혀 모른다 -- 그건 StepController의 책임이다.
//
// TODO(watch 담당자): PrintAll의 실제 출력 로직을 구현한다.
// - watchList의 각 이름을 executor.TryGetVariable로 조회
// - 찾으면 값을 포맷해서 출력(문자열/불리언/숫자/null 등, ExecutePrintStmt와
//   동일한 포맷이면 사용자 입장에서 일관적임), 못 찾으면 "<undefined>" 같이 표시
class WatchList {
public:
	void Add(const std::string& name);
	void Remove(const std::string& name);
	void PrintAll(const ExecutorUnit& executor) const;

private:
	std::vector<std::string> names; // 추가한 순서 유지
};
