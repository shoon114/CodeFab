# 데모 2 — 함수

## 2-1. 재귀 + 여러 줄 선언

```
func fact(n)
{
  if (n <= 1) { return 1; }
  return n * fact(n - 1);
}
print fact(5);
```

출력: `120`. 함수 이름/파라미터와 `{`가 다른 줄에 있어도 정상 동작한다.

## 2-2. 함수는 호출자의 지역 변수를 볼 수 없다

```
func tryAccess() { return localOnly; }
{ var localOnly = 5; print tryAccess(); }
```

출력: `Undefined variable 'localOnly'` 런타임 에러. 함수는 전역 변수는 볼 수
있지만, 우연히 호출자가 같은 이름의 지역 변수를 갖고 있다고 해서 그걸 몰래
참조하면 안 된다 — 이 경계를 명확히 테스트로 고정해둔 사례.

## 2-3. 정적 오류가 나면 실행 자체를 하지 않는다

```
func add(a, b) { return a + b; }
print add(1);
```

출력: 인자 개수 불일치를 알리는 정적 오류 메시지 **한 줄**. 예전에는
`CheckerUnit`이 오류를 보고한 뒤에도 `ExecutorUnit`이 이어서 실행돼 같은
문제에 대해 정적/런타임 에러 메시지가 중복 출력됐었다 — 지금은 정적 오류가
있으면 실행 자체를 막는다.
