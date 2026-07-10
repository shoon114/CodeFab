# 데모 3 — 클래스 상속 + Super + 다형성

```javascript
Class Animal { speak() { return "..."; } describe() { return "I say " + this.speak(); } }
Class Dog : Animal { speak() { return "Woof, and " + Super.speak(); } }
var d = Dog();
print d.describe();
```

출력: `I say Woof, and ...`

포인트:
- `Dog`는 `speak()`을 오버라이드하면서도 `Super.speak()`으로 부모(`Animal`)의
  구현을 호출할 수 있다.
- `describe()`는 `Dog`에 정의돼 있지 않지만 상속받은 `Animal`의 구현이
  그대로 동작하고, 그 안의 `this.speak()`은 **호출 시점의 실제 인스턴스
  타입**(`Dog`)에 맞는 오버라이드를 부른다 — 다형성이 정확히 지켜진다.
- `Super.method(...)`는 "인스턴스의 실제 클래스"가 아니라 "현재 실행 중인
  메서드가 정의된 클래스의 부모"부터 탐색을 시작하도록 구현되어 있어서,
  오버라이드한 메서드를 다시 호출하는 무한 재귀에 빠지지 않는다.
