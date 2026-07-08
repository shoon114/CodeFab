#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>

class SyntaxNode;

struct FunctionObject {
    std::vector<std::string> parameters;
    SyntaxNode* body = nullptr;
};

// ExecutorUnit이 다루는 런타임 값.
// 이름을 Value_t로 둔 이유: 전역 이름 Value는 gmock의 testing::Value()와
// using namespace testing; 사용 시 충돌(ambiguous)한다.
using Value_t = std::variant<double, std::string, bool, std::shared_ptr<FunctionObject>>;
