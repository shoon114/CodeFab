# 데모 2 — 함수는 호출자의 지역 변수를 볼 수 없다

```javascript
func tryAccess() { return localOnly; }
{ var localOnly = 5; print tryAccess(); }
```

출력: `Undefined variable 'localOnly'` 런타임 에러.

포인트:
- 함수는 **전역 변수는 참조할 수 있어야** 하지만, 우연히 호출자가 같은
  이름의 지역 변수를 갖고 있다고 해서 그걸 몰래 들여다볼 수 있으면 안
  된다. 이 경계(스코프가 "선언 위치" 기준으로 정적으로 묶이는지, 아니면
  "호출 시점"의 변수까지 새는지)를 명확히 테스트로 고정해뒀다.
- 관련해서, 정적 오류(예: 인자 개수 불일치)가 있으면 `CheckerUnit`이
  보고한 뒤 `ExecutorUnit`은 아예 실행되지 않는다 — 같은 문제에 대해
  정적 오류 메시지와 런타임 오류 메시지가 중복 출력되는 혼란을 없앴다.
