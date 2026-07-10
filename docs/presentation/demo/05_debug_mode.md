# 데모 5 — 인터랙티브 디버그 모드 (`debug` 명령)

`CodeFab.exe debug <file>`로 실행하면 문(statement) 경계마다 멈춰서
`step`/`next`/`break`/`watch`/`inspect` 같은 명령으로 실행을 들여다볼 수 있다.
`ExecutorUnit`은 디버깅 개념을 전혀 모르고(Observer 패턴으로 완전히 분리),
`DebugSession`이 문 경계 통보를 받아 정지 여부/명령 처리를 전담한다.

## 준비: `add.cf`

준비된 파일: [`debug_mode/add.cf`](debug_mode/add.cf) (타이핑 없이 바로 실행 가능)

```javascript
func add(a, b) {
  var result = a + b;
  return result;
}
print add(2, 3);
```

## 진행

```
$ CodeFab.exe debug add.cf
[DEBUG] Loading Source Code : add.cf
[DEBUG] Stop at line : 1 > func add(a, b) {
(debug) next
[DEBUG] Stop at line : 5 > print add(2, 3);
(debug) step
[DEBUG] Stop at line : 1 > func add(a, b) {
(debug) step
[DEBUG] Stop at line : 2 >   var result = a + b;
(debug) step
[DEBUG] Stop at line : 3 >   return result;
(debug) watch result
(debug) inspect
[LOCAL] result = 5 (number)
(debug) continue
5
Program finished.
```

포인트:
- `next`는 현재 depth보다 깊이 들어가지 않고 다음 문에서 멈춘다(`func` 선언
  자체는 건너뛰고 바로 `print` 호출로 이동).
- `step`은 함수 호출 내부까지 따라 들어간다.
- `watch <이름>`으로 등록한 변수는 이후 정지할 때마다 스코프(`[LOCAL]`/
  `[GLOBAL]`)와 함께 자동으로 보여주고, `inspect`는 watch 목록과 무관하게
  현재 스코프의 변수 전부를 즉시 덤프한다.
- 존재하지 않는 파일로 `debug`를 실행하면 크래시 없이 `File not found: ...`
  로 깔끔하게 종료된다.
