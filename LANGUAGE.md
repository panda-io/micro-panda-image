# Micro Panda Language Cheat Sheet

## Types

| Type | Alias | C type | Notes |
| --- | --- | --- | --- |
| `bool` | | `bool` | `true` / `false` |
| `i8` | | `int8_t` | |
| `u8` | `byte` | `uint8_t` | |
| `i16` | | `int16_t` | |
| `u16` | | `uint16_t` | |
| `i32` | `int` | `int32_t` | default integer literal type |
| `u32` | | `uint32_t` | |
| `i64` | | `int64_t` | |
| `u64` | | `uint64_t` | |
| `float` | | `float` | 32-bit only — no f64 |
| `q16` | `fixed` | `int32_t` | 16.16 fixed-point |
| `void` | | `void` | absent return |

**Strings** = `u8[]` slice. No dedicated string type.

## Variables

```mpd
var x: i32 = 0          # mutable, explicit type
val y: i32 = 1          # immutable binding (no reassign)
const MAX = 100         # compile-time constant (no storage)
const PI: float = 3.14  # constant with explicit type

var z := 42             # mutable, type inferred (i32)
val s := "hello"        # inferred as u8[]
```

`val` prevents rebinding only — field mutation through `val` bindings is allowed.
Using `=` without explicit type annotation requires `:=` — bare `var x = 42` is an error.

## Literals

```mpd
42        0xFF       0b1010     0o755      # integer, default i32
3.14      1.5e2                            # float, default float
1.5       2.0                             # fixed-point when target type is fixed/q16
"hello"                                   # u8[] slice
'''multi
line'''                                   # multi-line u8[] slice (triple quotes)
true    false                             # bool
null                                      # null ref (&T types only)
'A'                                       # byte literal (u8 = 65)
[1, 2, 3]                                 # array literal, inferred T[N]
```

Numeric underscores allowed: `1_000_000`, `0xFF80_0000`.

## Casting

```mpd
i32(expr)   u8(val)   float(x)   i64(n)   # target_type(expr)
```

No implicit conversions. All mixed-type expressions need explicit casts.

## Operators

```plaintext
+  -  *  /  %            arithmetic
&  |  ^  ~  <<  >>       bitwise (no implicit promotions)
==  !=  <  >  <=  >=    comparison
&&  ||  !                logical
++  --                   increment / decrement
+=  -=  *=  /=  %=      compound assign
&=  |=  ^=  <<=  >>=    compound bitwise assign
&expr                    address-of (take reference)
.                        member access
[]                       subscript
```

## Control flow

```mpd
if condition
    ...
else if other
    ...
else
    ...

while condition
    ...
    break
    continue

for i in 0..10          # range [0, 10)
    ...

for item in array       # iterate by value
    ...

for i, item in array    # with index
    ...

for &item in array      # by reference — mutate in place; item has type &T
    ...
```

## Match

```mpd
match value
    0:                          # integer arm
        ...
    Color.Red:                  # enum arm
        ...
    Circle(r):                  # tagged enum — destructure fields to locals
        ...
    Add(left, op, right):       # multiple fields
        ...
    _:                          # wildcard / default (no exhaustiveness check)
        ...
```

No `break` needed. Arms do not fall through.

## Functions

```mpd
fun name(param: Type, other: Type) ReturnType
    return value

fun greet(name: u8[])           # void — return type omitted
    console.print(name)

fun min<T>(a: T, b: T) T        # generic function
    if a < b
        return a
    return b
```

Call: `add(1, 2)` / `min<i32>(a, b)`

No function overloading. Use generics or distinct names.

### Function references

```mpd
var f: fun(u8)                  # function pointer (no return)
var g: fun(i32, i32) i32        # with return type

f = my_function                 # assign non-@extern, non-@inline function
f(42)                           # call through pointer
fun apply(cb: fun(i32) i32, x: i32) i32
    return cb(x)
```

## Classes

```mpd
class Point(var x: i32, var y: i32)   # constructor params with var/val become fields
    val label: u8[] = "pt"            # extra field with default value

    fun length() float
        return math.sqrt(float(x*x + y*y))

    fun move(dx: i32, dy: i32)
        x += dx
        y += dy

    fun copy_from(other: &Point)      # class params always &ClassName
        x = other.x
        y = other.y
```

**Instantiation at module scope** (global / static lifetime):

```mpd
val _origin := Point(0, 0)
val _engine: Engine               # default init (no constructor params)
```

**Instantiation inside a function** — must use allocator:

```mpd
fun setup(alloc: &Allocator)
    val p: &Point = alloc.allocate<Point>()
    p.x = 3
```

No inheritance. Use composition (embedded class fields) or tagged enums for polymorphism.

## Enums

```mpd
enum Color                          # plain — auto values 0, 1, 2...
    Red
    Green
    Blue

enum HttpStatus                     # value enum — explicit integers
    Ok = 200
    NotFound = 404

enum Shape                          # tagged enum — variants carry data
    Circle(radius: float)
    Rect(w: float, h: float)

var c: Color = Color.Red
var s: Shape = Shape.Circle(2.5)
```

Recursive tagged enums use `&T` fields to keep struct size fixed:

```mpd
enum Expr
    Num(value: i32)
    Add(left: &Expr, right: &Expr)
```

## Arrays & Slices

```mpd
var buf: u8[256]            # fixed array — inline storage, compile-time size
var data: u8[]              # slice — fat pointer {ptr, size}, runtime size
var grid: i32[4][4]         # 2-D fixed array
var arr := [1, 2, 3]        # array literal → i32[3]

buf.size()                  # compile-time constant (256)
data.size()                 # runtime field
data[0]                     # subscript
var p: &u8 = &buf[4]        # address of element

val s: u8[] = buf           # coerce fixed array to slice
val sub: u8[] = {ptr, len}  # slice literal
```

Fixed arrays and slices are NOT interchangeable in general. Fixed arrays coerce to
slices automatically at function call sites.

## References

```mpd
var x: i32 = 10
var r: &i32 = &x    # take address
r = 20              # assign through reference (x becomes 20)
```

Class types in function parameters are always `&ClassName` — no implicit struct copies.
`null` is compatible with any `&T` reference type.

## Generics

Functions only (not classes). Monomorphized at each call site.

```mpd
fun wrap<T>(v: &T) &T
    return v

sizeof<T>()             # byte size of T (compile-time in generic context)
&T(expr)                # cast expr to void*, typed at call site
```

Call: `wrap<Point>(&p)` → generates `wrap_Point`.

## Annotations

```mpd
@extern("sinf({x})")        # inline C expression; {param} → call-site arg
fun sin(x: float) float

@extern                     # call by same C name unchanged
fun tick()

@extern("my_c_var")         # zero-arg, no placeholder → emit as expression (variable access)
fun get_flag() bool

@inline                     # emit static inline; no stable address
fun fast(x: i32) i32
    return x * 2

@test                       # collected by test runner; no params, no return
fun test_add()
    assert(1 + 1 == 2)

@raw("#include <stdio.h>")  # verbatim C at top of generated file
@raw('''
#include <stdlib.h>
#include <string.h>
''')

@interface                  # platform stub — prototype only, no body, no emit
fun gpio_set(pin: i32, high: bool)
```

`@extern` and `@inline` functions cannot be used as function references.

## Imports

```mpd
import console                      # qualifier: console.print(...)
import util.math                    # nested path: math.min(...)
import util.math::min               # symbol import: min(...) directly
import util.math::*                 # wildcard: all public symbols
import util.math as m               # module alias: m.min(...)
import util.math::min as minimum    # symbol alias: minimum(...)
```

## Preprocessor

```mpd
#if HOSTED
    // desktop-only code
#else
    // other code
#end

#if HOSTED || MCU32
    // either target
#end
```

Flags declared in `mpd.yaml` under `targets.<name>.flags`.

## Built-ins

```mpd
assert(expr)            # abort with source location if false
sizeof<T>()             # byte size of T at compile time
sizeof(T)               # same, classic form
```

## Visibility

`_name` → private (C `static` linkage, file-local)
`name` → public

## C name mangling

| Micro Panda | C output |
| --- | --- |
| `module::function` | `module__function` |
| class method `Foo.bar` | `Foo_bar` |
| generic `min<i32>` | `min_int32_t` |
| private `_helper` in `mod` | `static mod___helper` |
