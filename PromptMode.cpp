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

		try {
			TokenList tokenList = tokenizer.CreateTokenForCode();
			trees.push_back(assembler.Parse(tokenList));
			// 정적 오류가 있으면 ReportError가 이미 stderr에 보고했으므로, 잘못된
			// 트리를 실행해 런타임 오류를 또 내지 않도록 여기서 멈춘다.
			if (checker.Check(trees.back().get())) {
				executor.Execute(*trees.back());
				std::cout << std::endl;
			}
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
	// 한다. 다만 그 확인이 끝났는데도 여전히 체인을 연장하는 else가 없다면 더
	// 기다리지 않고 바로 실행해야 한다 -- 그러지 않으면 else 없는 if 뒤에 무관한
	// 문장이 계속 이어질 때 영원히 대기하게 된다.
	//
	// 단순히 "한 번 기다렸는지" bool로는 부족하다 -- "if (...) 문장; else if (...)
	// 문장;"처럼 조건과 body가 한 줄에 합쳐진 스타일에서는, 첫 if의 body가 닫혀서
	// 한 번 기다린 뒤 "else if (...) 문장;"이 통째로 들어오면 그 시점에 else-if의
	// body도 이미 닫혀 있어 다시 "이 지점은 처음 보는 대기 상태"로 취급해야 하는데,
	// bool 하나로는 "이미 한 번 기다렸다"는 상태가 그대로 이어져 버려서 진짜
	// 마지막 순수 else를 기다리지 못하고 성급하게 실행돼 버린다. 그래서 "마지막으로
	// 대기를 시작했을 때 버퍼에 있던 else 개수"를 기록해두고, 이후 else 개수가
	// 늘어났다면(=새로운 else/else-if가 실제로 체인을 연장했다면) 그 시점을
	// 다시 "새로 대기를 시작하는 시점"으로 취급한다. else 개수가 그대로라면(=
	// 아무것도 연장되지 않았다면) 더 기다리지 않는다.
	int elseCountAtLastWait = -1;
	// 다음 반복에서 새 줄을 cin으로부터 읽지 않고, 이번에 이미 읽은 line을
	// 그대로 재사용해야 하면 true로 설정한다(아래 "체인이 연장되지 못했다"
	// 분기 참고).
	bool reuseLine = false;
	while (true) {
		if (reuseLine) {
			reuseLine = false;
		} else {
			// buffer가 비어있으면 새 문장의 시작(">>> "), 그렇지 않으면 이전 줄에서
			// 이어지는 멀티라인 입력 중("..> ")이라는 뜻이다. stdout이 아니라
			// stderr로 보내는 이유: stdout으로 보내면 파이프/리다이렉션으로 출력을
			// 캡처하는 system test(run.ps1)에서 프롬프트 문자열이 실제 실행 결과와
			// 같은 줄에 섞여 기대값 비교가 깨진다. 프롬프트는 UI 장식이지 프로그램의
			// 실행 결과가 아니므로 stdout과 분리한다.
			std::cerr << (buffer.empty() ? ">>> " : "..> ") << std::flush;
			if (!std::getline(std::cin, line)) {
				break;
			}
			if (line == "exit" || line == "quit") {
				break;
			}
			if (buffer.empty() && line.empty()) {
				continue;
			}
		}

		// "else 대기" 상태(elseCountAtLastWait >= 0)에서 받은 다음 줄이 체인을
		// 연장하지 못하면, 그 줄은 이미 완결된 앞의 if-체인과는 무관한 별도의
		// 문장이다. 이 줄을 buffer에 합치기 전 상태를 남겨둬서, 그런 경우 완결된
		// 앞부분만 먼저 실행하고 이번 줄은 새 buffer로 처음부터 다시 검사할 수
		// 있게 한다(합쳐서 하나의 트리로 실행해버리면, 앞부분 실행 중 예외가
		// 났을 때 뒤에 붙은 멀쩡한 문장이 예외 없이 통째로 실행되지 않고
		// 사라지는 문제가 있었다).
		std::string bufferBeforeThisLine = buffer;
		bool wasWaitingForElse = (elseCountAtLastWait >= 0);

		buffer += line + "\n";

		Tokenizer tokenizer;
		std::istringstream bufferInput(buffer);
		std::streambuf* originalCinBuffer = std::cin.rdbuf(bufferInput.rdbuf());
		tokenizer.GetCodeFromUser();
		std::cin.rdbuf(originalCinBuffer);

		TokenList tokenList;
		try {
			tokenList = tokenizer.CreateTokenForCode();
		} catch (const std::exception& e) {
			// import 파일을 못 여는 등, 아직 문장이 완성되기도 전에 토큰화 단계에서
			// 실패할 수 있다. 이 경우 미완성 판단 로직까지 갈 필요 없이 바로 에러를
			// 보고하고 버퍼를 비운 뒤 다음 입력을 받는다.
			std::cerr << e.what() << std::endl;
			buffer.clear();
			elseCountAtLastWait = -1;
			continue;
		}

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
		bool chainFailedToExtend = false;
		if (tokenList.size() >= 2) {
			TokenType firstType = tokenList[0].type;
			TokenType lastRealType = tokenList[tokenList.size() - 2].type;
			if (lastRealType == TokenType::KwElse) {
				pendingControlBlock = true; // "... else" 다음에 '{' 또는 'if'를 기다리는 중
				elseCountAtLastWait = -1; // 이미 'else'를 만났으니 "혹시 else?" 대기 상태는 아니다
			} else if (lastRealType == TokenType::RParen
				&& (firstType == TokenType::KwIf || firstType == TokenType::KwFor || firstType == TokenType::KwFunc)) {
				pendingControlBlock = true; // "if (...)"/"for (...)"/"func name(...)" 다음에 '{'를 기다리는 중
				elseCountAtLastWait = -1;
			} else if (firstType == TokenType::KwClass && lastRealType == TokenType::Identifier) {
				// "Class Name"이나 "Class Name : Parent"까지만 입력된 상태 --
				// 클래스 body('{...}')는 반드시 있어야 하므로(단일 문장으로
				// 대체될 수 없음) 다음 줄의 '{'를 기다린다. 이미 닫힌 선언은
				// 마지막 토큰이 RBrace이므로 이 조건에 걸리지 않는다.
				pendingControlBlock = true;
				elseCountAtLastWait = -1;
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
				int elseCount = 0;
				for (size_t i = 0; i < tokenList.size(); i++) {
					if (tokenList[i].type == TokenType::KwElse) {
						elseCount++;
						bool isElseIf = (i + 1 < tokenList.size() && tokenList[i + 1].type == TokenType::KwIf);
						if (!isElseIf) {
							sawTerminalElse = true;
						}
					}
				}
				if (!sawTerminalElse) {
					// "if (...) 문장; else if (...) 문장;"처럼 조건과 body가 한
					// 줄에 합쳐진 스타일에서는, else-if의 body까지 이미 닫힌
					// 채로 한 번에 들어올 수 있다. 이 경우 else 개수가 지난번
					// 대기 시점보다 늘어났으므로(=실제로 체인이 한 칸 더
					// 연장됐으므로) "새로 대기를 시작하는 시점"으로 취급해서
					// 한 번 더 기다린다. else 개수가 그대로라면(=이번에 새로
					// 추가된 줄이 체인을 전혀 연장하지 못했다면) 더 기다리지
					// 않는다.
					if (elseCount > elseCountAtLastWait) {
						pendingControlBlock = true;
						elseCountAtLastWait = elseCount;
					} else {
						elseCountAtLastWait = -1;
						chainFailedToExtend = true;
					}
				} else {
					elseCountAtLastWait = -1;
				}
			} else {
				elseCountAtLastWait = -1;
			}
		}
		if (pendingControlBlock) {
			continue;
		}

		if (wasWaitingForElse && chainFailedToExtend) {
			// 방금 읽은 줄은 else 체인을 연장하지 못했다 -- bufferBeforeThisLine에
			// 담긴, 이미 완결된 if(-else if)* 체인과는 무관한 별도의 문장이다.
			// 합쳐서 하나의 트리로 실행하면 앞부분 실행 중 예외가 났을 때 뒤에
			// 붙은 이 문장이 함께 버려지므로, 완결된 부분만 먼저 실행하고 이번
			// 줄은 새 buffer로 처음부터 다시 검사한다(그 줄 자체가 또 다른
			// 여러 줄짜리 문장의 시작일 수도 있으므로 line을 재사용해 루프를
			// 한 번 더 돈다).
			runBuffer(bufferBeforeThisLine);
			buffer.clear();
			reuseLine = true;
			continue;
		}

		elseCountAtLastWait = -1;
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
