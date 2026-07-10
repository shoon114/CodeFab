# 데모 1 — 스코프 & 제어문

콘솔(Release build)에 아래 순서대로 붙여넣으면서 진행한다. 모두 실제 실행으로
검증된 코드다.

## 1-1. 블록 스코프 (변수 가리기)

```
var x = "global";
{
  var x = "inner";
  print x;
}
print x;
```

출력: `inner` 다음 `global`. 안쪽 블록의 `x`는 블록을 벗어나면 사라지고,
바깥의 `x`가 다시 보인다.

## 1-2. 여러 줄(Allman 스타일) if/else if/else

```
var a = 9;
var b = 0;
if (a > 3)
{
  print "x";
}
else if (b > 1)
{
  print "y";
}
else
{
  print "z";
}
```

출력: `x`. 조건과 `{`가 다른 줄에 있어도 정상 동작한다 (예전에는 `func`/`Class`
포함 이 스타일 전체가 깨졌었다 — `docs/presentation_edge_cases.md` 1번 참고).

## 1-3. 조건+본문이 한 줄에 합쳐진 3단 else-if/else 체인

```
var a = 1;
var b = 5;
if (a > 3) print "x";
else if (b > 1) print "y";
else print "z";
```

출력: `y`. **이 입력 스타일은 한때 "Unexpected token" 에러로 깨졌던 케이스다**
(`docs/presentation_edge_cases.md` 2번). `else`와 `else if`를 구분하지 못해서
생긴 버그였고, 지금은 고쳐져 있다는 걸 보여주는 데모.

## 1-4. for 문의 스코프 누수 방지

```
for (var i = 0; i < 3; i = i + 1) { print i; }
print i;
```

출력: `012` 다음 `Undefined variable 'i'` 런타임 에러. `for`의 init에서 선언한
변수(`i`)는 반복문이 끝나면 사라져야 한다는 걸 보여주는 데모 — 이것도 한때는
새지 않고 바깥에 계속 남아있던 버그였다.
