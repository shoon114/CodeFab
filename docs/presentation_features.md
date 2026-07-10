# CodeFab 언어 기능

> 발표 원본 자료. 데모용 코드는 `docs/presentation/demo/`에 기능별로 분리되어 있다.

## 변수와 스코프

```javascript
var x = "global";
{
  var x = "inner";
  print x;   // inner
}
print x;     // global
```

- `{}` 블록마다 새 스코프가 생기고, 안쪽 변수는 블록을 벗어나면 사라진다.
- 같은 스코프 안에서 같은 이름을 두 번 선언하면 정적 오류.
- 초기화식에서 자기 자신을 참조하면(`var a = a;`) 정적 오류.

## 제어문 (if / for)

```javascript
if (a > 3) { print "x"; }
else if (b > 1) { print "y"; }
else { print "z"; }

for (var i = 0; i < 3; i = i + 1) { print i; }
```

- 조건/파라미터와 `{}` body가 서로 다른 줄에 걸쳐 있어도 된다(Allman 스타일).
- `{}` 없이 단일 문장 body도 허용된다: `for (var j = 0; j < 3; j = j + 1) print j;`

## 함수

```javascript
func fact(n) {
  if (n <= 1) { return 1; }
  return n * fact(n - 1);
}
print fact(5);   // 120
```

- 재귀 호출 지원.
- 함수는 전역 변수는 참조할 수 있지만, 호출자의 로컬 변수는 참조할 수 없다.
- `return` 없이 끝나면 `null`을 반환한다.
- 함수 선언도 이름/파라미터와 body가 서로 다른 줄에 걸쳐 있어도 된다.

## 배열

```javascript
var arr = Array(3);
for (var i = 0; i < 3; i = i + 1) { arr[i] = i * i; }
print arr[2];   // 4
```

- `Array(size)`로 생성, 초기값은 `null`.
- 인덱스 범위/음수 검사, 배열이 아닌 값 인덱싱 검사 등 런타임 검증.

## 클래스

```javascript
Class Animal {
  speak() { return "..."; }
  describe() { return "I say " + this.speak(); }
}
Class Dog : Animal {
  speak() { return "Woof, and " + Super.speak(); }
}
var d = Dog();
print d.describe();   // I say Woof, and ...
```

- `init`은 생성자로 취급된다.
- 필드는 `this.field = value`로 선언 없이 동적으로 생성된다.
- 상속(`:`), 메서드 오버라이딩, `Super.method(...)`로 부모 구현 호출 지원.
- 클래스 외부에서 `this`/`Super` 사용, 자기 자신 상속, 정의되지 않은
  클래스 상속은 모두 정적 오류로 잡는다.

## import

```javascript
import "math_utils.cf" alias math;
```

- import 대상 파일의 코드를 토큰 스트림에 그대로 삽입해 실행한다.
- 반복문 내부에서는 import를 사용할 수 없다(정적 오류).
- 같은 스코프 내 중복 import, 별칭 이름 충돌도 정적으로 검사한다.
