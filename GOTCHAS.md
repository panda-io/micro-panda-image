# Micro Panda — Common Pitfalls

## 1. No implicit numeric conversions

Every type conversion must be explicit. Mixed-type expressions are compile errors.

```mpd
var a: i32 = 10
var b: i64 = a          // ERROR — must cast
var b: i64 = i64(a)     // OK
var f: float = a        // ERROR
var f: float = float(a) // OK
```

## 2. `u8[]` (slice) ≠ `u8[N]` (fixed array)

These are completely different types. You cannot pass a fixed array where a slice is expected and vice versa — except at function call sites, where a fixed array is automatically coerced to a slice.

```mpd
var buf: u8[256]   // fixed array — inline storage, size is compile-time constant
var s: u8[]        // slice — fat pointer {ptr, len}, runtime size

fun process(data: u8[])  // takes slice
    ...

process(buf)       // OK — fixed array coerced to slice at call site
var s: u8[] = buf  // OK — assignment coercion
```

Never try to return a local fixed array as a slice — the pointer will dangle.

## 3. Class instances inside functions require an allocator

You cannot create a class instance on the stack. Either:

- Declare at **module (global) scope** — lives in static memory
- Use an **allocator** inside a function

```mpd
// Global scope — OK (static lifetime)
val _point := Point(3, 4)

// Inside function — WRONG (no stack allocation for class types)
fun bad()
    val p := Point(3, 4)    // compile error

// Inside function — correct: use allocator
fun good(alloc: &Allocator)
    val p: &Point = alloc.allocate<Point>()
    p.x = 3
    p.y = 4
```

## 4. Class types must always be passed by reference

Function parameters of class type must use `&ClassName`. There are no implicit struct copies.

```mpd
fun process(p: Point)    // ERROR — must be by reference
fun process(p: &Point)   // OK
```

This applies to all class types and tagged enums when used as function parameters.
Field assignment (`other.field = value`) and embedded class fields are fine as value types.

## 5. `@extern` functions cannot be used as function references

`@extern` functions are expanded inline at every call site — they have no real C address.
Assigning an `@extern` function to a `fun(...)` variable is a compile error.

```mpd
@extern("sinf({x})")
fun sin(x: float) float

var f: fun(float) float = sin   // ERROR
```

## 6. `val` only prevents rebinding, not mutation

`val` stops you reassigning the binding. It does not make the data immutable.
Field writes and method calls through a `val` binding are still allowed.

```mpd
val p := Point(1, 2)
p.x = 10            // OK — mutating field through val binding
p = Point(3, 4)     // ERROR — rebinding a val
```

## 7. `fixed` / `q16` hex literals are scaled

Fixed-point hex literals mean `value * 65536`. `0xFFFF` in a `fixed` context is NOT `65535`
— it is `0xFFFF * 65536 = 4294901760`. Use decimal literals for clarity.

```mpd
var f: fixed = 1.5      // OK — 98304 raw (1.5 * 65536)
var m: fixed = -1.0     // bitwise floor mask (0xFFFF0000 as int32_t)
var bad: fixed = 0xFFFF // NOT what you want — huge number
```

## 8. `float` is 32-bit only — there is no `f64`

Micro Panda has one float type: `float` (32-bit). There is no `double` or `f64`.
6 significant decimal digits of precision. Trig / sqrt wrap 32-bit C functions (`sinf`, `sqrtf`).

## 9. Strings are `u8[]` slices — use `string.equals()` for comparison

`"hello" == other` compares the slice struct (pointer + length), not the content.
Use `string.equals(a, b)` for string equality.

```mpd
import string

if string.equals(name, "admin")
    ...
```

## 10. No function overloading

You cannot have two functions with the same name and different parameters.
Use generics or distinct names instead.

```mpd
fun process_i32(v: i32) ...
fun process_float(v: float) ...
// or:
fun process<T>(v: T) ...
```

## 11. `null` is only valid for reference types (`&T`)

`null` compiles to C `NULL` and is only type-compatible with `&T` references.
It cannot be used with value types, slices, or primitive types.

```mpd
var p: &Point = null    // OK
var x: i32 = null       // ERROR
```

## 12. Module access uses `.` but imports use `::`

```mpd
import console              // qualifier style: console.print(...)
import console::print       // symbol import: print(...) directly
import console::*           // wildcard: all public symbols imported

console.print("hi")         // dot for member access / module access
```

## 13. `@inline` functions also lack a stable address

Like `@extern`, `@inline` functions emit as `static inline` in C and should not be used
as function references.

## 14. Array literals infer element type, not slice type

```mpd
var arr := [1, 2, 3]        // type is i32[3], not i32[]
val s: i32[] = arr          // coerce to slice explicitly when needed
```

## 15. `for &item in array` — item is already dereferenced

In a reference for-in loop, `item` has type `&T`. Access fields with `item.field`
(not `item->field` — the compiler handles the pointer syntax).

```mpd
var data: i32[4] = [1, 2, 3, 4]
for &item in data
    item = 0    // mutates in place; item has type &i32
```
