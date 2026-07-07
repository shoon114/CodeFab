#pragma once
#include <string>
#include <variant>

// ExecutorUnit이 다루는 런타임 값. 현재는 숫자(double)와 문자열을 지원한다.
// 이름을 Value_t로 둔 이유: 전역 이름 Value는 gmock의 testing::Value()와
// using namespace testing; 사용 시 충돌(ambiguous)한다.
using Value_t = std::variant<double, std::string, bool>;
