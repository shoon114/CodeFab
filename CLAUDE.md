# CodeFab 개발 가이드 (Claude 세션용)

이 문서는 AssemblerUnit(파서 모듈) 개발 과정에서 사용자와 합의된 design decision과
앞으로의 개발 진행 방식을 정리한 것이다. 새로운 Claude 세션에서도 아래 내용을 그대로
따라야 한다.

## 1. 참고하면 안 되는 파일

- `src/skelton.cpp`: 개발자들 간 초안 공유용 임시 파일. 실제 빌드에 포함되지 않으며,
  설계 판단의 근거로 참고하지 말 것.
- `docs/CodeFab_Design.md`: 초기 설계 문서. 현재 코드(특히 `AssemblerUnit`/파서 구조)와
  어긋난 부분(예: `IExpressionParser` 생성자 주입 방식)이 있어 **최신 상태가 아님**.
  파서 아키텍처에 대해서는 이 문서(CLAUDE.md)를 우선 신뢰할 것.

## 2. AssemblerUnit 파서 아키텍처: Strategy + Registry

`AssemblerUnit::Parse()`는 각 statement 타입을 직접 분기(switch/if-else)하지 않는다.
모든 파서는 단일 인터페이스로 통일되어 있고, 전역 레지스트리를 통해 런타임에 필요한
파서를 얻어와 위임하는 구조다.

### 핵심 구성 요소

- **`IStatementParser`**: 유일한 파서 인터페이스.
  ```cpp
  class IStatementParser {
  public:
      virtual ~IStatementParser() = default;
      virtual std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList, size_t& pos) = 0;
  };
  ```
  (과거에는 `IExpressionParser`를 별도로 두고 `ExpressionParser`가 이를 구현했으나,
  `refactoring/AssemblerUnit/RemoveIExpressionParser` 브랜치에서 `IExpressionParser`를
  삭제하고 `ExpressionParser`도 `IStatementParser`를 구현하도록 통합했다. 이 브랜치가
  아직 master에 머지되지 않았다면, 작업 대상 브랜치가 어느 쪽인지 먼저 확인할 것.)

- **`StatementParserRegistry`**: leading `TokenType` → factory(`std::function<shared_ptr<IStatementParser>()>`)
  매핑을 갖는 전역 싱글턴(`Instance()`). `Register()`로 등록, `Resolve()`로 조회하며
  미등록 토큰은 `nullptr`을 반환한다(예외를 던지지 않음 — 호출자가 nullptr 체크 후
  `std::runtime_error`를 던지는 책임을 진다).

- **`StatementParserRegistrar<TParser>`**: 파서가 스스로를 registry에 등록하게 하는
  템플릿. 각 파서의 `.cpp` 파일 익명 namespace에 정적 객체로 선언한다.
  ```cpp
  namespace {
      StatementParserRegistrar<VarDeclareParser> registrar(TokenType::KwVar);
  }
  ```
  `ExpressionParser`처럼 여러 leading token(Number, String, Identifier, KwTrue, KwFalse,
  LParen, Minus, Not 등)에서 시작할 수 있는 파서는 토큰 타입 수만큼 registrar를 따로
  선언한다.

- **`AssemblerUnit`**: 생성자가 없다. `Parse()`는 첫 토큰 타입으로 registry에서 파서를
  `Resolve()`해서 위임할 뿐, 어떤 파서가 존재하는지 전혀 모른다(해당 파서의 헤더도
  include하지 않는다).

### 설계 원칙 (반드시 지킬 것)

1. **생성자 주입 금지**: 어떤 파서도 다른 특정 파서를 생성자로 주입받지 않는다.
   필요한 하위 파서는 항상 `StatementParserRegistry::Instance().Resolve(tokenType)`로
   실행 시점에 얻어온다.
2. **자기 등록(self-registration)**: 새 파서를 추가해도 `AssemblerUnit`이나 다른 파서의
   코드를 수정할 필요가 없다. 해당 파서의 `.cpp`에 `StatementParserRegistrar<T>` 한 줄만
   추가하면 된다. (Open-Closed Principle을 실질적으로 지키기 위한 결정 — 처음엔
   `AssemblerUnit` 생성자가 각 파서를 만들어 등록하는 방식이었으나, 새 statement가
   추가될 때마다 `AssemblerUnit.cpp`를 고쳐야 해서 폐기됨.)
3. **Resolve 실패 처리**: `Resolve()`가 `nullptr`을 반환하면, 호출한 파서가 문맥에 맞는
   메시지로 `std::runtime_error`를 던진다 (예: `"Expected a variable name after 'var' at line ..."`).

### 새 statement 파서를 추가하는 절차

1. `IStatementParser`를 구현하는 클래스 작성. 생성자 없음(또는 인자 없는 기본 생성자만).
2. `.cpp` 파일에 `namespace { StatementParserRegistrar<MyParser> registrar(TokenType::KwXxx); }`
   추가. 여러 leading token을 처리한다면 registrar를 여러 개 선언.
3. `Parse()` 내부에서 하위 파싱이 필요하면
   `StatementParserRegistry::Instance().Resolve(tokenList[pos].type)`로 해당 파서를 얻어
   위임. `nullptr`이면 `std::runtime_error` 던짐.
4. TC는 하위 파서 의존성을 `MockStatementParser`(gmock)로 stub해서 격리 검증한다.

## 3. 테스트(TC) 작성 규칙

- 다른 파서에 대한 의존성은 항상 `MockStatementParser`를 만들어
  `StatementParserRegistry::Instance().Register(TokenType::X, factory)`로 등록한 뒤
  stub/expect로 검증한다. 실제 구현체(`ExpressionParser` 등)에 의존하지 않는다.
- **레지스트리는 전역 싱글턴이므로 fixture의 `SetUp()`/`TearDown()`에서 반드시 아래
  패턴을 지킬 것** (그렇지 않으면 크래시/leak/타 테스트 오염이 발생했던 전례가 있음):

  ```cpp
  std::shared_ptr<MockStatementParser> mockTailParser = std::make_shared<MockStatementParser>();

  void SetUp() override {
      // this가 아니라 shared_ptr을 값으로 캡처한다. 이 람다는 전역 registry의
      // static map에 저장되어 fixture보다 오래 살아남기 때문에, this를 캡처하면
      // fixture 소멸 후 dangling pointer가 되어 다른 테스트 파일에서 크래시가 난다.
      StatementParserRegistry::Instance().Register(TokenType::Identifier,
          [tailParser = mockTailParser]() { return tailParser; });
  }

  void TearDown() override {
      // 캡처한 mock을 registry에서 반드시 해제한다. 안 그러면 프로그램 종료 시
      // "leaked mock object" 에러가 나거나, 등록을 남긴 채로 다음 테스트(다른 파일
      // 포함)가 이미 만족된 expectation을 가진 mock을 재사용하게 되어 실패한다.
      StatementParserRegistry::Instance().Register(TokenType::Identifier,
          []() { return nullptr; });
  }
  ```

- 이 패턴을 빠뜨린 사례가 실제로 있었음 (`TestIfStatementParser.cpp`가 한동안 `this`
  캡처 + `TearDown()` 누락 상태로 남아있었고, 전수 점검 중 발견하여 수정함). 새 테스트
  파일을 작성하거나 리뷰할 때 이 두 가지(값 캡처, TearDown 해제)를 항상 확인할 것.

## 4. 개발 프로세스 컨벤션

- 새 기능/버그 수정은 "설계 제안 → 사용자 확인 → TC 작성 → 구현 → 빌드/테스트 →
  커밋" 사이클로 진행하고, 논리적으로 구분되는 케이스마다 커밋을 분리한다.
- Git 히스토리를 다룰 때 squash-merge된 브랜치를 다시 rebase해야 하면 일반
  `git rebase master`(patch-id 기반 자동 스킵)가 아니라
  `git rebase --onto master <old-base-commit> <branch>`로 진짜 새 커밋만 replay한다.

## 5. 앞으로의 개발 단계 (2026-07 기준)

지금까지는 TDD로 각 모듈의 API를 유닛 테스트 수준까지 검증했다. 이제부터는 아래 두
트랙으로 작업을 진행한다.

1. **System test 단계**: 실제 사용자가 콘솔에 입력하는 코드에 대해 최종 실행 결과를
   확인하는 테스트(유닛 테스트가 아니라 end-to-end 실행 결과 검증)를 진행한다. 이
   과정에서 발견되는 버그를 고치거나 필요한 리팩토링을 하면서 커밋을 작성한다.
2. **신규 요구사항 기능 개발**: 새 기능을 구현할 때는 TDD 방식(유닛 테스트 선작성)을
   따르지 않는다. 대신 system test 수준(콘솔 입력 → 실행 결과)에서 바로 검증하면서
   개발을 진행한다.

즉, 앞으로 새로 작성하는 테스트는 기본적으로 "콘솔에 코드를 입력했을 때의 실행 결과"를
검증하는 system test이고, 기존에 확립된 유닛 테스트/mock 기반 TDD 컨벤션(위 2, 3절)은
이미 완료된 파서 모듈의 유지보수(버그 수정, 리팩토링) 시에만 참고한다.

**단, 기존에 정의된 유닛 테스트(TC)는 계속 살아있는 회귀 테스트다.** system test로
새 기능/버그 수정을 검증하더라도, 커밋 전에 기존 TC 전체(`CodeFab.exe` 실행 결과 전체
테스트 스위트)가 항상 PASS하는지 반드시 확인한다. system test 검증이 유닛 테스트 작성을
대체하는 것이지, 기존 유닛 테스트를 무시해도 된다는 뜻이 아니다.
