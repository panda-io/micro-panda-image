# Micro Panda Standard Library API

## `import console`

Terminal / UART output. All platforms.

```mpd
init(write_byte: fun(byte))          # set custom byte-output transport
write_byte(b: byte)                  # emit one byte
write_string(s: u8[])               # emit bytes of slice
write_bool(v: bool)                  # "true" or "false"
write_u8(v: u8)
write_u16(v: u16)
write_u32(v: u32)
write_u64(v: u64)
write_i8(v: i8)
write_i16(v: i16)
write_i32(v: i32)
write_i64(v: i64)
write_int(v: int)                    # alias for write_i32
write_float(v: float)                # 4 decimal places
write_q16(v: q16)                    # 16.16 fixed-point, 4 decimal places
write_fixed(v: fixed)                # alias for write_q16
print(s: u8[])                       # write_string + newline (\n)
print_args(s: u8[], args: int[])     # format string then print (see string.format)
```

Default transport: `putchar` on HOSTED/MCU32; must call `init()` on bare MCU.

---

## `import string`

String utilities. All platforms.

### Comparison / search

```mpd
equals(a: byte[], b: byte[]) bool
starts_with(string: byte[], prefix: byte[]) bool
ends_with(string: byte[], suffix: byte[]) bool
index_of(string: byte[], character: byte) int   # -1 if not found
```

### Slicing

```mpd
sub(string: byte[], start: int, len: int) byte[]
trim(string: byte[]) byte[]                      # strip leading/trailing whitespace
trim_start(string: byte[]) byte[]
trim_end(string: byte[]) byte[]
```

### Tokenization

```mpd
token(string: byte[], start: int, delim: byte) byte[]  # slice up to next delim
skip(string: byte[], start: int, delim: byte) int       # advance past delim chars
```

### Parsing

```mpd
parse_u32(string: byte[]) u32
parse_i32(string: byte[]) i32
parse_int(string: byte[]) int
```

### Formatting

```mpd
format_u32(buf: byte[], value: u32) int    # writes digits into buf, returns count
format_i32(buf: byte[], value: i32) i32
format_int(buf: byte[], value: int) int

format(text: byte[], buf: byte[], args: int[]) byte[]
```

`format` replaces `{Ni}` / `{Nu}` / `{Nf}` / `{Nd}` / `{Nb}` placeholders:

- `{0i}` — signed int (default)
- `{0u}` — unsigned int
- `{0f}` — float (pass bits via `string.float_bits(v)`)
- `{0d}` — q16 fixed-point (pass as `int(v)` or `string.fixed_bits(v)`)
- `{0b}` — bool ("true"/"false")

```mpd
var buf: byte[64]
var args: int[2] = {score, string.float_bits(ratio)}
val s := string.format("score={0i} ratio={1f}", buf, args)
console.write_string(s)
```

### Bit-cast helpers (for format args)

```mpd
float_bits(value: float) int      # reinterpret float bits as int (no value change)
q16_bits(value: q16) int          # reinterpret q16 as int
fixed_bits(value: fixed) int      # alias for q16_bits
```

---

## `import math`

Math functions and constants. HOSTED and MCU32.

### Constants

```mpd
const PI  := 3.14159265358979
const TAU := 6.28318530717959
const E   := 2.71828182845905
```

### Generic (all platforms)

```mpd
min<T>(a: T, b: T) T
max<T>(a: T, b: T) T
clamp<T>(value: T, low: T, high: T) T
abs<T>(value: T) T
```

### Fixed-point rounding

```mpd
floor_q16(value: q16) q16
ceil_q16(value: q16) q16
round_q16(value: q16) q16
floor_fixed(value: fixed) fixed    # alias
ceil_fixed(value: fixed) fixed
round_fixed(value: fixed) fixed
```

### Float (HOSTED / MCU32 only)

```mpd
sin(value: float) float
cos(value: float) float
tan(value: float) float
asin(value: float) float
acos(value: float) float
atan(value: float) float
atan2(y: float, x: float) float
sqrt(value: float) float
pow(base: float, exp: float) float
floor(value: float) float
ceil(value: float) float
round(value: float) float
```

---

## `import mcu32.allocator` (or `import mcu32.allocator::Allocator`)

Arena allocator. HOSTED and MCU32.

```mpd
class Allocator()
    init(mem: byte[])                        # point at backing buffer
    allocate<T>() &T                         # returns null on overflow
    allocate_array<T>(length: int) T[]       # returns {null,0} on overflow
    available() int                          # bytes remaining
    reset()                                  # free all (reset cursor to 0)
```

Usage pattern:

```mpd
var _mem: byte[4096]
val _alloc: Allocator

fun main()
    _alloc.init(_mem)
    val p: &Point = _alloc.allocate<Point>()
```

---

## `import mcu32.collection`

Fixed-capacity collections backed by `Allocator`. HOSTED and MCU32.
Requires `import mcu32.allocator`.

### `ArrayList<T>`

```mpd
class ArrayList<T>()
    init(alloc: &Allocator, capacity: int) bool
    push(value: T) bool          # false if full
    pop() bool                   # false if empty
    insert(i: int, value: T) bool
    get(i: int) T
    set(i: int, value: T)
    top() T                      # last element
    size() int
    capacity() int
    is_empty() bool
    is_full() bool
    clear()
```

### `RingBuffer<T>`

```mpd
class RingBuffer<T>()
    init(allocator: &Allocator, capacity: int) bool
    push(value: T) bool          # false if full
    pop() bool                   # false if empty; removes front
    peek() T                     # front element, no remove
    size() int
    capacity() int
    is_empty() bool
    is_full() bool
```

---

## `import hosted.allocator` (or `::HeapAllocator`)

Heap allocator wrapping malloc/realloc/free. **HOSTED only.**

```mpd
class HeapAllocator()
    allocate_array<T>(len: int) T[]
    realloc_array<T>(array: T[], new_length: int) T[]
    free_array<T>(array: T[])
```

No `init()` needed — stateless wrapper. Declare global or pass by reference.

---

## `import hosted.collection`

Growable collections backed by `HeapAllocator`. **HOSTED only.**

### `HeapList<T>`

```mpd
class HeapList<T>()
    init(heap: &HeapAllocator)
    push(value: T) bool
    pop() bool
    insert(i: int, value: T) bool
    get(i: int) T
    set(i: int, value: T)
    top() T
    size() int
    capacity() int
    is_empty() bool
    clear()
    free()
```

### `HeapMap<T>` — string-keyed hash map

```mpd
class HeapMap<T>()
    init(heap: &HeapAllocator)
    contains(key: byte[]) bool
    get(key: byte[]) T              # undefined if key absent — check contains() first
    set(key: byte[], value: T)
    remove(key: byte[]) bool
    size() int
    free()
```

---

## `import hosted.file`

File I/O. **HOSTED only.**

```mpd
const READ        = "r"
const WRITE       = "w"
const APPEND      = "a"
const READ_WRITE  = "r+"
const WRITE_READ  = "w+"
const APPEND_READ = "a+"

class File()
    open(path: byte[], mode: byte[]) bool
    close()
    is_open() bool
    read_bytes(buf: byte[]) int        # bytes actually read
    write_bytes(buf: byte[]) int       # bytes actually written
    write_string(s: byte[])
    read_line(buf: byte[]) int         # reads until \n or EOF; returns byte count
    flush()
    seek(pos: int)
    tell() int
```

---

## `import hosted.args`

Access to `argc`/`argv`. **HOSTED only.**

```mpd
arg_count() int          # total argument count (includes program name at [0])
arg_value(i: int) byte[] # argument i as u8[] slice
```

---

## `import hosted.time`

Timing utilities. **HOSTED only.**

```mpd
sleep_us(us: u32)        # sleep N microseconds
time_us() i64            # monotonic clock, microseconds since some epoch
```

---

## `import hosted.signal`

SIGINT / SIGTERM handler. **HOSTED only.**

```mpd
catch_signals()          # install handler for SIGINT, SIGTERM (and SIGHUP on POSIX)
exit_requested() bool    # true after a signal is received
```

Usage:

```mpd
import hosted.signal

fun main()
    signal.catch_signals()
    while !signal.exit_requested()
        // main loop
```
