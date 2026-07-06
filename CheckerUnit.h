#pragma once
#include "SyntaxTree.h"

class CheckerUnit {
public:
    virtual ~CheckerUnit() = default;

    // 계약:
    //  - tree는 null이 아니어야 한다 (호출부 책임).
    //  - 타입/스코프/의미 오류가 있으면 CheckError(line, column)를 던진다.
    //  - 통과 시 부작용 없음 (tree를 변형하지 않음).
    virtual void Check(SyntaxTree& tree) = 0;
};
