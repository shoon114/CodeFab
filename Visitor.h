#pragma once
#include "SyntaxTree.h"
class Visitor {
public:
    virtual ~Visitor() = default;
    virtual void Visit(SyntaxTree& node) = 0;
};