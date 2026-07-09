#include "PromptMode.h"
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "Tokenizer.h"
#include "AssemblerUnit.h"
#include "CheckerUnit.h"
#include "ExecutorUnit.h"

int PromptMode::Run() {
	AssemblerUnit assembler;
	CheckerUnit checker;
	ExecutorUnit executor;

	// 함수 선언(FuncDeclStmt)은 FunctionObject::body에 body 노드를 raw pointer로만
	// 보관한다. 그 노드가 속한 tree가 사라지면 이후 줄에서 함수를 호출할 때 dangling
	// pointer가 되므로, 파싱된 tree는 매 줄이 끝나도 버리지 않고 세션이 끝날 때까지
	// 계속 붙잡아둔다(변수처럼 executor.scopes도 REPL 전체에 걸쳐 유지되는 것과 동일한 이유).
	std::vector<std::unique_ptr<SyntaxNode>> trees;

	// buffer를 토큰화/파싱/실행하고 비운다. 파싱/실행 중 예외가 나면 stderr에
	// 보고한다(구문 오류든 런타임 오류든 이 한 곳에서 처리).
	auto runBuffer = [&](std::string& buffer) {
		Tokenizer tokenizer;
		std::istringstream bufferInput(buffer);
		std::streambuf* originalCinBuffer = std::cin.rdbuf(bufferInput.rdbuf());
		tokenizer.GetCodeFromUser();
		std::cin.rdbuf(originalCinBuffer);

		TokenList tokenList = tokenizer.CreateTokenForCode();

		try {
			trees.push_back(assembler.Parse(tokenList));
			checker.Check(trees.back().get());
			executor.Execute(*trees.back());
			std::cout << std::endl;
		} catch (const std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
		buffer.clear();
	};

	// 줄 단위로 입력을 누적하다가, 누적된 버퍼를 토큰화했을 때 '{'/'}' 개수가
	// 맞아떨어지는 시점(블록이 전부 닫힌 시점)에만 그 버퍼 전체를 토큰화/파싱/실행한다.
	// 이렇게 하지 않으면 블록이 여러 줄에 걸친 입력(if/for 등)이 첫 줄만으로
	// 파싱을 시도하다가 '}'를 못 찾고 실패한다.
	// Tokenizer::GetCodeFromUser()는 std::cin을 rdbuf()로 통째로 읽으므로,
	// 매번 std::cin을 누적 버퍼만 담은 istringstream으로 잠시 바꿔치기해서 호출한다.
	std::string buffer;
	std::string line;
	// if가 '{}' 없는 단일 문장 body를 가질 때(예: "if (a > 3)\nprint "x";"), body가
	// ';'로 끝나는 순간에도 'else'가 이어질 수 있어 한 번은 다음 줄을 더 확인해야
	// 한다. 다만 그 확인이 끝났는데도(=이미 한 번 대기했는데도) 여전히 'else'가
	// 없다면 더 기다리지 않고 바로 실행해야 한다 -- 그러지 않으면 else 없는 if
	// 뒤에 무관한 문장이 계속 이어질 때 영원히 대기하게 된다. 이 상태를 버퍼가
	// 실행되거나 else 체인에 확실히 들어선 시점마다 리셋해가며 추적한다.
	bool waitedOnceForElse = false;
	while (std::getline(std::cin, line)) {
		if (line == "exit" || line == "quit") {
			break;
		}
		if (buffer.empty() && line.empty()) {
			continue;
		}
		buffer += line + "\n";

		Tokenizer tokenizer;
		std::istringstream bufferInput(buffer);
		std::streambuf* originalCinBuffer = std::cin.rdbuf(bufferInput.rdbuf());
		tokenizer.GetCodeFromUser();
		std::cin.rdbuf(originalCinBuffer);

		TokenList tokenList = tokenizer.CreateTokenForCode();

		int braceDepth = 0;
		for (const Token& token : tokenList) {
			if (token.type == TokenType::LBrace) {
				braceDepth++;
			} else if (token.type == TokenType::RBrace) {
				braceDepth--;
			}
		}
		if (braceDepth > 0) {
			continue; // 블록이 아직 안 닫혔으니 다음 줄을 더 받는다
		}

		// "if (true)"처럼 '{'가 아직 한 번도 나오지 않은 채로 끝난 줄은 브레이스
		// 개수만 보면 0(균형)이라 "완성된 문장"으로 착각하기 쉽지만, 실제로는
		// 다음 줄에 올 '{'를 기다리는 중인 미완성 상태다. 이 상태에서 그대로
		// 파싱을 시도하면 예외가 나서 버퍼가 통째로 비워지고, 뒤이어 입력되는
		// '{ ... }' 블록이 앞의 if/for 조건과 분리된 채 무조건 실행되어 버린다.
		// 이 판단은 버퍼 전체에 '{'가 한 번이라도 나왔는지가 아니라, 버퍼의 마지막
		// 토큰이 어떤 모양인지로만 해야 한다 -- 예를 들어
		// "if (a > 3) { print "x"; } else if (b > 1)"처럼 앞부분에 이미 완결된
		// '{...}' 블록이 있어도, 마지막의 "else if (...)"는 여전히 자기 블록을
		// 기다리는 중인 미완성 상태다.
		//
		// 다만 "블록을 기다리는 중"인지 판단하는 조건은 좁게 잡아야 한다. 처음엔
		// "'{'도 없고 ';'로 끝나지도 않았으면 무조건 대기"로 했었는데, 이러면
		// "print 1 + 2"처럼 단순히 ';'을 빼먹은 진짜 구문 오류까지도 계속
		// 대기 상태에 빠져서(대화형 콘솔에서는 EOF가 오지 않으므로) 아무 반응
		// 없이 멈춘 것처럼 보이는 문제가 있었다. if/for의 조건 '(...)'가 막
		// 끝났거나, else가 막 끝난 경우처럼 "다음이 반드시 '{'여야 하는" 패턴만
		// 좁게 감지해서 그 경우에만 대기한다.
		bool pendingControlBlock = false;
		if (tokenList.size() >= 2) {
			TokenType firstType = tokenList[0].type;
			TokenType lastRealType = tokenList[tokenList.size() - 2].type;
			if (lastRealType == TokenType::KwElse) {
				pendingControlBlock = true; // "... else" 다음에 '{' 또는 'if'를 기다리는 중
				waitedOnceForElse = false; // 이미 'else'를 만났으니 "혹시 else?" 대기 상태는 아니다
			} else if (lastRealType == TokenType::RParen
				&& (firstType == TokenType::KwIf || firstType == TokenType::KwFor)) {
				pendingControlBlock = true; // "if (...)"/"for (...)" 다음에 '{'를 기다리는 중
				waitedOnceForElse = false;
			} else if (firstType == TokenType::KwIf
				&& (lastRealType == TokenType::RBrace || lastRealType == TokenType::Semicolon)) {
				// if의 body가 방금 닫혔다(브레이스 body든 '{}' 없는 단일 문장
				// body든). 이 if(또는 이 if의 마지막 'else if' 분기)를 마저
				// 연장하는 'else'/'else if'가 다음 줄에 올 수도 있으므로 한 줄 더
				// 받아서 확인해야 한다.
				//
				// 단순히 "버퍼에 'else'가 하나라도 있는지"로는 부족하다 --
				// "if ... else if ... else if ... else ..." 체인에서 앞의 'else
				// if'들은 이미 자기 몫의 else를 가졌지만, 체인이 아직 안 끝난
				// '순수 else(뒤에 if가 없는 else)'가 한 번도 안 나왔다면 그
				// 체인은 여전히 연장될 수 있다. 반대로 순수 else가 이미 한 번
				// 나왔다면(문법상 그 뒤에 또 else가 올 수 없으므로) 더 이상
				// 기다릴 필요가 없다.
				bool sawTerminalElse = false;
				for (size_t i = 0; i < tokenList.size(); i++) {
					if (tokenList[i].type == TokenType::KwElse) {
						bool isElseIf = (i + 1 < tokenList.size() && tokenList[i + 1].type == TokenType::KwIf);
						if (!isElseIf) {
							sawTerminalElse = true;
							break;
						}
					}
				}
				if (!sawTerminalElse) {
					// 다만 이미 한 번 확인했는데도(waitedOnceForElse) 여전히
					// 체인을 연장하는 else가 없다면 더 기다리지 않고 지금
					// 실행한다 -- 안 그러면 else 없는 if 뒤에 무관한 문장이
					// 계속 이어질 때 영원히 대기하게 된다.
					if (!waitedOnceForElse) {
						pendingControlBlock = true;
						waitedOnceForElse = true;
					} else {
						waitedOnceForElse = false;
					}
				} else {
					waitedOnceForElse = false;
				}
			} else {
				waitedOnceForElse = false;
			}
		}
		if (pendingControlBlock) {
			continue;
		}

		waitedOnceForElse = false;
		runBuffer(buffer);
	}

	// 입력(EOF)이 끝났는데도 아직 실행되지 않은 버퍼가 남아있을 수 있다
	// (예: 세미콜론을 빼먹은 채로 끝난 구문 오류, 혹은 닫히지 않은 블록).
	// 이 경우 위 루프의 "미완성이면 계속 버퍼링" 판단 때문에 조용히 버려지고
	// 아무 에러도 보고되지 않을 수 있으므로, 남은 버퍼가 있으면 마지막으로
	// 한 번 더 파싱을 시도해 에러든 실행 결과든 보고한다.
	if (!buffer.empty()) {
		runBuffer(buffer);
	}

	return 0;
}
