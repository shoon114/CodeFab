#include "WatchList.h"
#include "ExecutorUnit.h"
#include <algorithm>
#include <sstream>

namespace {

	// 값 하나를 (타입 표시 없이) 출력한다. 배열이면 원소마다 재귀 호출해 대괄호로 펼친다.
	void PrintValue(std::ostream& out, const Value_t& value) {
		if (std::holds_alternative<bool>(value)) {
			out << (std::get<bool>(value) ? "true" : "false");
		} else if (std::holds_alternative<std::string>(value)) {
			out << "\"" << std::get<std::string>(value) << "\"";
		} else if (std::holds_alternative<std::monostate>(value)) {
			out << "null";
		} else if (std::holds_alternative<std::shared_ptr<ArrayObject>>(value)) {
			const auto& elements = std::get<std::shared_ptr<ArrayObject>>(value)->elements;
			out << "[";
			for (size_t i = 0; i < elements.size(); ++i) {
				if (i > 0) {
					out << ", ";
				}
				PrintValue(out, elements[i]);
			}
			out << "]";
		} else {
			out << std::get<double>(value);
		}
	}

	// PrintValue가 출력한 값 뒤에 붙는 "(number)"/"(Boolean)" 같은 타입 표시.
	std::string TypeNameOf(const Value_t& value) {
		if (std::holds_alternative<bool>(value)) {
			return "Boolean";
		}
		if (std::holds_alternative<std::string>(value)) {
			return "string";
		}
		if (std::holds_alternative<std::shared_ptr<ArrayObject>>(value)) {
			return "array";
		}
		return "number";
	}

}  // namespace

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

std::string WatchList::Watches(const ExecutorUnit& executor) const {
	std::ostringstream out;
	for (const std::string& name : names) {
		// "d[0]"처럼 인덱스가 붙은 이름은 배열 변수(baseName)를 찾은 뒤 그 원소값만 가리킨다.
		size_t bracketPos = name.find('[');
		std::string baseName = (bracketPos == std::string::npos) ? name : name.substr(0, bracketPos);

		Value_t value;
		if (!executor.TryGetVariable(baseName, value)) {
			out << name << " = <undefined>" << std::endl;
			continue;
		}

		bool isLocal = executor.CurrentScope().count(baseName) > 0;

		if (bracketPos != std::string::npos) {
			if (!std::holds_alternative<std::shared_ptr<ArrayObject>>(value)) {
				out << name << " = <undefined>" << std::endl;
				continue;
			}
			const auto& elements = std::get<std::shared_ptr<ArrayObject>>(value)->elements;
			size_t index = static_cast<size_t>(std::stoul(name.substr(bracketPos + 1)));
			if (index >= elements.size()) {
				out << name << " = <undefined>" << std::endl;
				continue;
			}
			value = elements[index];
		}

		out << (isLocal ? "[LOCAL] " : "[GLOBAL] ") << name << " = ";
		PrintValue(out, value);
		out << " (" << TypeNameOf(value) << ")" << std::endl;
	}
	return out.str();
}

std::string WatchList::Inspect(const ExecutorUnit& executor) const {
	std::ostringstream out;
	for (const auto& entry : executor.CurrentScope()) {
		out << "[LOCAL] " << entry.first << " = ";
		PrintValue(out, entry.second);
		out << " (" << TypeNameOf(entry.second) << ")" << std::endl;
	}
	return out.str();
}
