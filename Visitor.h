#pragma once
#include "SyntaxNode.h"
class Visitor {
public:
    virtual ~Visitor() = default;
    virtual void Visit(SyntaxNode& node) = 0;
};