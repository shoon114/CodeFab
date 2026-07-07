// Checker Unit(SyntaxNode 트리 검사기)에 대한 GoogleTest 단위 테스트.
// 트리는 Assembler Unit 없이 tests/SyntaxNodeBuilders.h로 손으로 조립한다.
#include <gtest/gtest.h>

#include "Checker.h"
#include "CodeFabException.h"
#include "SyntaxNodeBuilders.h"

using namespace testutil;

namespace {

void RunCheck(SyntaxNode& root) {
    Checker checker;
    checker.Check(root);
}

} // namespace

TEST(CheckerTest, DuplicateDeclarationInSameBlockThrows) {
    auto root = program(nodes(
        block(nodes(
            varDecl("a", 1, numberLiteral("10", 1)),
            varDecl("a", 2, numberLiteral("20", 2))))));

    EXPECT_THROW(RunCheck(*root), CheckError);
}

TEST(CheckerTest, SelfReferenceInInitializerThrows) {
    auto root = program(nodes(
        block(nodes(
            varDecl("a", 2, binary(identifier("a", 2), TokenType::Plus, "+", 2, numberLiteral("1", 2)))))));

    EXPECT_THROW(RunCheck(*root), CheckError);
}

TEST(CheckerTest, SameNameInNestedScopeIsAllowed) {
    // { var a = 1; { var a = 2; } }
    auto root = program(nodes(
        block(nodes(
            varDecl("a", 1, numberLiteral("1", 1)),
            block(nodes(
                varDecl("a", 2, numberLiteral("2", 2))))))));

    EXPECT_NO_THROW(RunCheck(*root));
}

TEST(CheckerTest, ReferencingOtherAlreadyDefinedVariableIsAllowed) {
    // { var a = 10; var b = a + 1; }
    auto root = program(nodes(
        block(nodes(
            varDecl("a", 1, numberLiteral("10", 1)),
            varDecl("b", 2, binary(identifier("a", 2), TokenType::Plus, "+", 2, numberLiteral("1", 2)))))));

    EXPECT_NO_THROW(RunCheck(*root));
}

TEST(CheckerTest, GlobalScopeDuplicateDeclarationIsAllowedByPolicy) {
    // Program(Global) 레벨은 정책상 중복 선언 검사 대상이 아니다 (docs/Checker.md 6장 참고).
    auto root = program(nodes(
        varDecl("a", 1, numberLiteral("1", 1)),
        varDecl("a", 2, numberLiteral("2", 2))));

    EXPECT_NO_THROW(RunCheck(*root));
}

TEST(CheckerTest, EmptyProgramPasses) {
    auto root = program({});
    EXPECT_NO_THROW(RunCheck(*root));
}

TEST(CheckerTest, VarDeclWithoutInitializerPasses) {
    auto root = program(nodes(block(nodes(varDecl("a", 1)))));
    EXPECT_NO_THROW(RunCheck(*root));
}

TEST(CheckerTest, DeclaringAfterSiblingNestedBlockClosesIsAllowed) {
    // { { var a = 1; } var a = 2; } -- 안쪽 블록이 닫힌 뒤 바깥 스코프에서 처음 선언
    auto root = program(nodes(
        block(nodes(
            block(nodes(varDecl("a", 1, numberLiteral("1", 1)))),
            varDecl("a", 2, numberLiteral("2", 2))))));

    EXPECT_NO_THROW(RunCheck(*root));
}

TEST(CheckerTest, CheckErrorReportsLineAndColumnOfDuplicateDeclaration) {
    auto root = program(nodes(
        block(nodes(
            varDecl("a", 1, numberLiteral("1", 1), /*column=*/3),
            varDecl("a", 5, nullptr, /*column=*/9)))));

    try {
        RunCheck(*root);
        FAIL() << "CheckError가 던져지지 않았습니다.";
    } catch (const CheckError& e) {
        EXPECT_EQ(e.line, 5);
        EXPECT_EQ(e.column, 9);
    }
}

TEST(CheckerTest, CheckerInstanceIsReusableAfterThrowingError) {
    // Prompt Shell처럼 Checker 인스턴스를 여러 줄에 걸쳐 재사용해도
    // 이전 호출의 예외 때문에 ScopeStack이 깨지면 안 된다.
    Checker checker;

    auto badRoot = program(nodes(
        block(nodes(
            varDecl("a", 1, numberLiteral("1", 1)),
            varDecl("a", 2, numberLiteral("2", 2))))));
    EXPECT_THROW(checker.Check(*badRoot), CheckError);

    auto goodRoot = program(nodes(block(nodes(varDecl("b", 3, numberLiteral("3", 3))))));
    EXPECT_NO_THROW(checker.Check(*goodRoot));
}
