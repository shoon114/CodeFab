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

struct ArrayObject;

// ExecutorUnit이 다루는 런타임 값.
// 이름을 Value_t로 둔 이유: 전역 이름 Value는 gmock의 testing::Value()와
// using namespace testing; 사용 시 충돌(ambiguous)한다.
// std::monostate는 배열의 초기화되지 않은 칸(null)을 나타낸다.
using Value_t = std::variant<double, std::string, bool, std::shared_ptr<FunctionObject>, std::monostate, std::shared_ptr<ArrayObject>>;

// Arr(size)로 생성되는 고정 크기 배열. shared_ptr로 다뤄야 변수 대입/전달 시
// 배열이 값 복사가 아니라 참조(동일 객체 공유) 의미를 갖는다.
struct ArrayObject {
    std::vector<Value_t> elements;
};
