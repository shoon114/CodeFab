#pragma once
#include "Visitor.h"

class SyntaxTree {
public:
    virtual ~SyntaxTree() = default;
    virtual void Accept(Visitor& visitor) = 0;
};
