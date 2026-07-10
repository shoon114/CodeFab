# 데모 3 — 배열

## 3-1. 생성/인덱싱

```
var arr = Array(3);
for (var i = 0; i < 3; i = i + 1) { arr[i] = i * i; }
print arr[2];
```

출력: `4`. `Array(size)`로 생성하며 초기값은 `null`이다.

## 3-2. 범위를 벗어난 인덱싱은 런타임 에러

```
var arr = Array(3);
print arr[5];
```

출력: `Array index out of range` 런타임 에러. 배열이 아닌 값을 인덱싱하거나
음수 인덱스를 쓰는 경우도 동일하게 검증되어 있다 — 흔히 놓치기 쉬운 경계
조건을 명시적으로 처리한 부분.
