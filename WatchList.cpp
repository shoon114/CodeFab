#include "WatchList.h"
#include "ExecutorUnit.h"
#include <algorithm>
#include <iostream>

void WatchList::Add(const std::string& name) {
	if (std::find(names.begin(), names.end(), name) == names.end()) {
		names.push_back(name);
	}
}

void WatchList::Remove(const std::string& name) {
	names.erase(std::remove(names.begin(), names.end(), name), names.end());
}

bool WatchList::Contains(const std::string& name) const {
	return std::find(names.begin(), names.end(), name) != names.end();
}

void WatchList::PrintAll(const ExecutorUnit& executor) const {
	// TODO(watch 담당자): names를 순회하며 executor.TryGetVariable로 값을 조회해 출력
	for (const std::string& name : names) {
		Value_t value;
		if (executor.TryGetVariable(name, value)) {
			std::cout << name << " = " << std::get<double>(value) << std::endl;
		} else {
			std::cout << name << " = <undefined>" << std::endl;
		}
	}
}
