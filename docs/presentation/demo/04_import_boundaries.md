# 데모 4 — import 경계 처리 (선언만 들여오기 + 파일 오류 catch)

`import`는 대상 파일의 **선언(함수/변수/클래스)만** 가져오고, 그 파일에 있는
`print`/`if`/`for` 같은 부수효과(side effect) 문장은 실행하지 않는다.

준비된 파일: [`import_boundaries/`](import_boundaries/) (타이핑 없이 바로 실행 가능)

## `import_boundaries/math_utils.cf`

```javascript
print "this should NOT print during import";
func square(x) {
  return x * x;
}
```

## `import_boundaries/import_demo.cf`

```javascript
import "math_utils.cf" alias math;
print math.square(5);
```

## 실행 결과

```
$ cd docs/presentation/demo/import_boundaries
$ CodeFab.exe run import_demo.cf
25
```

`math_utils.cf`의 `print` 문은 **한 번도 출력되지 않는다** — import 도중에는
"선언이 아닌 동작"을 억제하도록 `ExecutorUnit`이 처리하기 때문이다.
`square` 함수만 `math` 네임스페이스로 들어와 `math.square(5)`로 정상 호출된다.

## 존재하지 않는 파일을 import하면 (`import_boundaries/import_missing.cf`)

```javascript
import "does_not_exist.cf" alias x;
print 1;
```

```
$ CodeFab.exe run import_missing.cf
Cannot open import file: does_not_exist.cf at line 0
```

크래시 없이 명확한 에러 메시지로 종료된다 — "다른 파일을 가져온다"는
기능이 파일 시스템 경계에서도 깨지지 않게 방어되어 있다는 포인트.
