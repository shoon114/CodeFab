#pragma once

class SyntaxNode;

// ExecutorUnit이 Stmt 경계마다 통보하는 이벤트를 구독하는 인터페이스(Observer).
// ExecutorUnit은 이 인터페이스만 알고 "디버깅"이라는 개념 자체는 모른다.
class ExecutionObserver {
public:
	virtual ~ExecutionObserver() = default;
	virtual void OnStmtEnter(const SyntaxNode& node) = 0;
	virtual void OnStmtExit(const SyntaxNode& node) = 0;
};
