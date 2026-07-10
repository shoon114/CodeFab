#ifdef _DEBUG
#include "gmock/gmock.h"
#include "StepController.h"

using namespace testing;

class StepControllerTest : public Test {
protected:
	StepController sc;
};

// ── 초기 상태 ────────────────────────────────────────────────────────────────

TEST_F(StepControllerTest, InitialMode_AlwaysPauses) {
	// 기본 모드는 Stepping이므로 어떤 줄·깊이에서도 정지해야 한다.
	EXPECT_TRUE(sc.ShouldPause(0, 1));
	EXPECT_TRUE(sc.ShouldPause(5, 3));
}

TEST_F(StepControllerTest, InitialBreakpoints_Empty) {
	EXPECT_TRUE(sc.Breakpoints().empty());
}

// ── Step ─────────────────────────────────────────────────────────────────────

TEST_F(StepControllerTest, Step_PausesAtEveryStatement) {
	sc.Continue(); // Running 으로 전환해 두고
	sc.Step();     // 다시 Stepping으로 복귀
	EXPECT_TRUE(sc.ShouldPause(0, 1));
	EXPECT_TRUE(sc.ShouldPause(10, 5));
}

// ── Next ─────────────────────────────────────────────────────────────────────

TEST_F(StepControllerTest, Next_PausesWhenDepthAtOrBelowRecorded) {
	sc.Next(2); // depth 2에서 Next 호출
	EXPECT_TRUE(sc.ShouldPause(0, 1)); // depth 1 ≤ 2 → 정지
	EXPECT_TRUE(sc.ShouldPause(0, 2)); // depth 2 ≤ 2 → 정지
}

TEST_F(StepControllerTest, Next_SkipsWhenDepthAboveRecorded) {
	sc.Next(2);
	EXPECT_FALSE(sc.ShouldPause(0, 3)); // depth 3 > 2 → 통과
	EXPECT_FALSE(sc.ShouldPause(0, 10));
}

// ── Continue ─────────────────────────────────────────────────────────────────

TEST_F(StepControllerTest, Continue_DoesNotPauseWithoutBreakpoint) {
	sc.Continue();
	EXPECT_FALSE(sc.ShouldPause(0, 1));
	EXPECT_FALSE(sc.ShouldPause(5, 3));
}

TEST_F(StepControllerTest, Continue_PausesAtBreakpointLine) {
	sc.SetBreakpoint(3);
	sc.Continue();
	EXPECT_TRUE(sc.ShouldPause(3, 1));
}

TEST_F(StepControllerTest, Continue_DoesNotPauseAtNonBreakpointLine) {
	sc.SetBreakpoint(3);
	sc.Continue();
	EXPECT_FALSE(sc.ShouldPause(4, 1));
}

// ── Breakpoint 관리 ───────────────────────────────────────────────────────────

TEST_F(StepControllerTest, SetBreakpoint_AppearsInBreakpoints) {
	sc.SetBreakpoint(5);
	EXPECT_THAT(sc.Breakpoints(), Contains(5));
}

TEST_F(StepControllerTest, SetBreakpoint_MultipleLines) {
	sc.SetBreakpoint(2);
	sc.SetBreakpoint(7);
	auto bp = sc.Breakpoints();
	EXPECT_THAT(bp, Contains(2));
	EXPECT_THAT(bp, Contains(7));
	EXPECT_EQ(bp.size(), 2u);
}

TEST_F(StepControllerTest, SetBreakpoint_DuplicateIgnored) {
	sc.SetBreakpoint(4);
	sc.SetBreakpoint(4);
	EXPECT_EQ(sc.Breakpoints().size(), 1u);
}

TEST_F(StepControllerTest, RemoveBreakpoint_RemovesExisting) {
	sc.SetBreakpoint(6);
	sc.RemoveBreakpoint(6);
	EXPECT_TRUE(sc.Breakpoints().empty());
}

TEST_F(StepControllerTest, RemoveBreakpoint_NonExistentIsNoOp) {
	sc.SetBreakpoint(6);
	sc.RemoveBreakpoint(99); // 없는 줄 제거 → 기존 breakpoint 유지
	EXPECT_EQ(sc.Breakpoints().size(), 1u);
}

// ── IsRunningMode ─────────────────────────────────────────────────────────────

TEST_F(StepControllerTest, IsRunningMode_TrueAfterContinue) {
	sc.Continue();
	EXPECT_TRUE(sc.IsRunningMode());
}

TEST_F(StepControllerTest, IsRunningMode_FalseInInitialSteppingMode) {
	EXPECT_FALSE(sc.IsRunningMode());
}

TEST_F(StepControllerTest, IsRunningMode_FalseAfterNext) {
	sc.Next(1);
	EXPECT_FALSE(sc.IsRunningMode());
}

TEST_F(StepControllerTest, IsRunningMode_FalseAfterStepFromRunning) {
	sc.Continue();
	sc.Step();
	EXPECT_FALSE(sc.IsRunningMode());
}

// ── 모드 전환 ─────────────────────────────────────────────────────────────────

TEST_F(StepControllerTest, ModeTransition_ContinueThenStep_ResumesSteppingBehavior) {
	sc.Continue();
	EXPECT_FALSE(sc.ShouldPause(0, 1)); // Running

	sc.Step();
	EXPECT_TRUE(sc.ShouldPause(0, 1));  // Stepping 복귀
}

TEST_F(StepControllerTest, ModeTransition_NextThenContinue_BreakpointOnlyPauses) {
	sc.Next(2);
	sc.Continue();
	// Running 모드이므로 breakpoint 없으면 정지 안 함
	EXPECT_FALSE(sc.ShouldPause(0, 1));
	EXPECT_FALSE(sc.ShouldPause(0, 2));
}

#endif
