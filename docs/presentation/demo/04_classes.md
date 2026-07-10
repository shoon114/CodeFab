# 데모 4 — 클래스

## 4-1. 상속 + 메서드 오버라이딩 + Super 호출

```
Class Animal { speak() { return "..."; } describe() { return "I say " + this.speak(); } }
Class Dog : Animal { speak() { return "Woof, and " + Super.speak(); } }
var d = Dog();
print d.describe();
```

출력: `I say Woof, and ...`. `Dog`가 `speak()`을 오버라이드하면서도
`Super.speak()`으로 부모(`Animal`)의 구현을 호출할 수 있다는 걸 보여준다.
`describe()`는 `Dog`에 정의되어 있지 않지만 상속받은 `Animal`의 구현이
그대로 동작하고, 그 안에서 `this.speak()`은 (정적 타입이 아니라) **실제
인스턴스의 타입**(`Dog`)에 맞는 오버라이드를 호출한다 — 다형성이 정확히
지켜지고 있다는 포인트.

## 4-2. (필요 시) 정적으로 잡히는 클래스 관련 오류들

말로 설명하며 넘어가도 좋은 포인트 (시간이 남으면 직접 입력해서 보여줘도 됨):

- 클래스 외부에서 `this`/`Super` 사용 → 정적 오류
- 자기 자신을 상속(`Class A : A`) → 정적 오류
- 정의되지 않은 클래스 상속 → 정적 오류
