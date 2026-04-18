# Micro Panda — Agent Quick Start

## What is it?

Micro Panda (`.mpd`) is a statically-typed systems language that compiles to C.
Targets: desktop (HOSTED), ESP32 / Cortex-M (MCU32). No heap, no GC, no implicit conversions.
Syntax is indentation-based (Python-style) — no braces, no `end` keywords.

## Toolchain

- Compiler: `mpd` binary (default install: `~/.local/bin/mpd`)
- Every project needs an `mpd.yaml` at its root

## CLI commands

| Command | Description |
| --- | --- |
| `mpd init` | Create `mpd.yaml` + `src/main.mpd` in current directory |
| `mpd build` | Parse → generate C → compile binary |
| `mpd build <target>` | Build a specific named target |
| `mpd run <target>` | Build then execute |
| `mpd test` | Compile and run all `*_test.mpd` files |
| `mpd gen` | Generate C only (no compile) |
| `mpd clean` | Delete `out/` and `bin/` |
| `mpd target add <name> <template>` | Add a target from template |

Templates: `hosted-debug`, `hosted-release`, `esp32-debug`, `esp32-release`

## mpd.yaml structure

```yaml
name: myapp
version: 0.1.0

targets:
  main:
    entry: main        # entry module (maps to src/main.mpd)
    src: src/          # source folder
    type: bin          # bin = compile to executable; c = generate C only
    flags: [HOSTED]    # preprocessor flags passed to #if blocks
    output: bin/main   # output binary path
    test: test/        # test folder (enables mpd test); discovers *_test.mpd
    cc:
      bin: gcc
      flags: [-g, -O0, -Wall]
```

For ESP32 (generate C only, hand off to idf.py):

```yaml
  esp32:
    entry: firmware/main
    src: src/
    type: c
    flags: [MCU32]
    out: main/firmware.c
    build_cmd: idf.py build
    gen:
      entry: app_main
```

## Conditional compile flags

| Flag | Meaning |
| --- | --- |
| `HOSTED` | Desktop (Linux / macOS / Windows) |
| `MCU32` | 32-bit MCU (ESP32, Cortex-M) |
| `DEBUG` | Debug build |

Use in source:

```mpd
#if HOSTED
    // desktop-only code
#else
    // MCU code
#end

#if HOSTED || MCU32
    // either target
#end
```

## Standard library — import paths

All stdlib modules are built-in (no `deps:` needed):

| Import | Available | Contents |
| --- | --- | --- |
| `import console` | all | Terminal/UART output |
| `import string` | all | String utilities, formatting |
| `import math` | HOSTED / MCU32 | Math functions and constants |
| `import mcu32.allocator` | HOSTED / MCU32 | Arena `Allocator` |
| `import mcu32.collection` | HOSTED / MCU32 | `ArrayList<T>`, `RingBuffer<T>` |
| `import hosted.allocator` | HOSTED | `HeapAllocator` (malloc/realloc/free) |
| `import hosted.collection` | HOSTED | `HeapList<T>`, `HeapMap<T>` |
| `import hosted.file` | HOSTED | `File` class, I/O |
| `import hosted.args` | HOSTED | `arg_count()`, `arg_value(i)` |
| `import hosted.time` | HOSTED | `sleep_us()`, `time_us()` |
| `import hosted.signal` | HOSTED | SIGINT / SIGTERM handler |

Import styles:

```mpd
import console                  # module qualifier: console.print(...)
import console::print           # specific symbol: print(...) directly
import console::*               # all public symbols imported directly
import math as m                # alias: m.sqrt(...)
import mcu32.allocator::Allocator  # specific class
```

## Typical project layout

```plaintext
myproject/
  mpd.yaml
  src/
    main.mpd          ← entry point (contains fun main())
    module_a.mpd      ← other source modules
  test/
    module_a_test.mpd ← test files (discovered by mpd test)
  bin/                ← compiled binaries (add to .gitignore)
  out/                ← generated C    (add to .gitignore)
```

## Minimal entry point

```mpd
import console

fun main()
    console.print("Hello, world!")
```

## Visibility rule

Names starting with `_` are private (file-local, C `static`). All other names are public.

## Compilation pipeline

```plaintext
*.mpd → compiler (Dart) → out/<target>.c → gcc → bin/<target>
```

`mpd gen` stops after writing the C file.
`mpd build` continues through to compile.

## See also

- `LANGUAGE.md` — complete language syntax cheat sheet
- `STDLIB.md` — standard library API reference
- `GOTCHAS.md` — common pitfalls
- `examples/` — runnable example programs
- `starter/` — copy-paste project template
