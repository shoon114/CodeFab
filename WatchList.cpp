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
		// "d[0]"처럼 인덱스가 붙은 이름은 배열 변수(baseName)를 찾은 뒤 그 원소값만 가리킨다.
		size_t bracketPos = name.find('[');
		std::string baseName = (bracketPos == std::string::npos) ? name : name.substr(0, bracketPos);

		Value_t value;
		if (!executor.TryGetVariable(baseName, value)) {
			std::cout << name << " = <undefined>" << std::endl;
			continue;
		}

		bool isLocal = executor.CurrentScope().count(baseName) > 0;

		if (bracketPos != std::string::npos) {
			if (!std::holds_alternative<std::shared_ptr<ArrayObject>>(value)) {
				std::cout << name << " = <undefined>" << std::endl;
				continue;
			}
			const auto& elements = std::get<std::shared_ptr<ArrayObject>>(value)->elements;
			size_t index = static_cast<size_t>(std::stoul(name.substr(bracketPos + 1)));
			if (index >= elements.size()) {
				std::cout << name << " = <undefined>" << std::endl;
				continue;
			}
			value = elements[index];
		}

		std::cout << (isLocal ? "[LOCAL] " : "[GLOBAL] ") << name << " = ";
		if (std::holds_alternative<bool>(value)) {
			std::cout << (std::get<bool>(value) ? "true" : "false") << " (Boolean)";
		} else if (std::holds_alternative<std::string>(value)) {
			std::cout << "\"" << std::get<std::string>(value) << "\" (string)";
		} else if (std::holds_alternative<std::shared_ptr<ArrayObject>>(value)) {
			const auto& elements = std::get<std::shared_ptr<ArrayObject>>(value)->elements;
			std::cout << "[";
			for (size_t i = 0; i < elements.size(); ++i) {
				if (i > 0) {
					std::cout << ", ";
				}
				const Value_t& element = elements[i];
				if (std::holds_alternative<bool>(element)) {
					std::cout << (std::get<bool>(element) ? "true" : "false");
				} else if (std::holds_alternative<std::string>(element)) {
					std::cout << "\"" << std::get<std::string>(element) << "\"";
				} else if (std::holds_alternative<std::monostate>(element)) {
					std::cout << "null";
				} else {
					std::cout << std::get<double>(element);
				}
			}
			std::cout << "] (array)";
		} else {
			std::cout << std::get<double>(value) << " (number)";
		}
		std::cout << std::endl;
	}
}
