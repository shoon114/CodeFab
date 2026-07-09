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
	for (const std::string& name : names) {
		Value_t value;
		if (executor.TryGetVariable(name, value)) {
			bool isLocal = executor.CurrentScope().count(name) > 0;
			std::cout << (isLocal ? "[LOCAL] " : "[GLOBAL] ") << name << " = ";
			if (std::holds_alternative<bool>(value)) {
				std::cout << (std::get<bool>(value) ? "true" : "false") << " (Boolean)";
			} else {
				std::cout << std::get<double>(value) << " (number)";
			}
			std::cout << std::endl;
		} else {
			std::cout << name << " = <undefined>" << std::endl;
		}
	}
}
