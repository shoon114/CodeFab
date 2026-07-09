# Library Import Design

## Goal

CodeFab supports importing declaration-only library files into the currently executing scope.

```codefab
import "sum.txt" alias sum;

var a = sum.add(1, 2);
```

The imported file is exposed as a module namespace value bound to the alias name. Functions and global variables declared in the imported file are available through member access on that alias.

## Syntax

```text
import <string-literal-path> alias <identifier>;
```

Rules:

- The path must be a string literal.
- The `alias` keyword is required.
- The alias must be an identifier.
- The statement must end with `;`.

## Parser Model

`ImportStatementParser` parses the import statement into `ImportStmtNode`.

`ImportStmtNode` stores:

- `token`: the `import` keyword token.
- `pathToken`: the string literal path token.
- `aliasToken`: the alias identifier token.

The expression parser also supports module member calls:

```codefab
sum.add(1, 2)
```

This is represented as:

```text
CallExpr
  MemberAccessExpr(sum, add)
  NumberLiteral(1)
  NumberLiteral(2)
```

## Checker Rules

The checker validates scope-level import rules:

- `import` is allowed in global and block scopes.
- `import` is not allowed inside a `for` loop body.
- The same file path cannot be imported again in the same scope or any ancestor scope.
- The alias name cannot collide with an existing variable, function, or class in the current scope.
- A valid import alias is registered as a variable-like symbol so later references can resolve.

Imported file contents are checked again when the executor loads the file.

## Runtime Model

At execution time, an import:

1. Resolves the file path relative to the current importing file, or the process working directory for REPL input.
2. Rejects circular imports using the active import stack.
3. Parses and checks the target file.
4. Executes only declaration-like statements from the imported file:
   - function declarations
   - global variable declarations
   - nested imports
5. Captures those declarations into a `ModuleObject`.
6. Binds the module object to the alias in the current scope.

The imported declarations are scoped through the alias only. They do not leak as direct names into the importing scope.

## Runtime Values

`ModuleObject` contains:

- `path`: resolved import path.
- `members`: map from declaration name to runtime value.

`FunctionObject` contains:

- parameter names
- function body AST pointer
- optional closure map for imported module globals

Imported functions capture the imported module members, so they can read imported global variables and call other imported declarations.

## Errors

The implementation reports runtime errors for:

- invalid import target file path
- duplicate import in current or ancestor scope
- circular import
- alias name collision
- import inside a loop
- member/function lookup failure on an imported module

