#include "AssemblerUnit.h"
#include "SyntaxTree.h"

class AssemblyUnitIf : public AssemblerUnit {
public:
    TokenList Tokenize(const std::string& source) override {
        TokenList tokens;
        return tokens;
    }

};