#include "WatchList.h"
#include <algorithm>

void WatchList::Add(const std::string& name) {
	if (std::find(names.begin(), names.end(), name) == names.end()) {
		names.push_back(name);
	}
}

void WatchList::Remove(const std::string& name) {
	names.erase(std::remove(names.begin(), names.end(), name), names.end());
}

void WatchList::PrintAll(const ExecutorUnit& executor) const {
	// TODO(watch 담당자): names를 순회하며 executor.TryGetVariable로 값을 조회해 출력
	(void)executor;
}
