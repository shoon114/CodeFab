#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class SyntaxNode;

struct FunctionObject {
    std::vector<std::string> parameters;
    SyntaxNode* body = nullptr;
};

struct ArrayObject;
struct InstanceObject;
struct ModuleObject;

// ExecutorUnit이 다루는 런타임 값.
// 이름을 Value_t로 둔 이유: 전역 이름 Value는 gmock의 testing::Value()와
// using namespace testing; 사용 시 충돌(ambiguous)한다.
// std::monostate는 배열의 초기화되지 않은 칸(null)을 나타낸다.
using Value_t = std::variant<double, std::string, bool, std::shared_ptr<FunctionObject>, std::monostate, std::shared_ptr<ArrayObject>, std::shared_ptr<InstanceObject>, std::shared_ptr<ModuleObject>>;

// Arr(size)로 생성되는 고정 크기 배열. shared_ptr로 다뤄야 변수 대입/전달 시
// 배열이 값 복사가 아니라 참조(동일 객체 공유) 의미를 갖는다.
struct ArrayObject {
    std::vector<Value_t> elements;
};

// Class 인스턴스. 필드는 동적으로(첫 대입 시) 생성되는 이름->값 사전이다.
// shared_ptr로 다뤄야 변수 대입/전달 시 인스턴스가 값 복사가 아니라 참조 의미를 갖는다
// (r.speed = 10;처럼 인스턴스를 통해 필드를 바꾸면 그 인스턴스를 참조하는 모든 변수에서 보여야 함).
struct InstanceObject {
    std::string className;
    std::unordered_map<std::string, Value_t> fields;
};

// import "경로" alias 별칭;으로 만들어지는 네임스페이스 값. alias를 통해
// sum.add(...)처럼 접근하는 함수/전역 변수를 이름->값으로 담는다. shared_ptr로
// 다뤄야 다른 값들과 동일하게 참조 의미를 갖는다.
struct ModuleObject {
    std::unordered_map<std::string, Value_t> members;
};
