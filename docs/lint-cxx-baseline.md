# clang-tidy + clazy baseline (Q2)

Date: 2026-07-09 · Tools: clang-tidy / clazy-standalone from Qt Creator's
bundled LLVM 21.1.2 · Scope: the 34 project TUs in the debug preset's
`compile_commands.json` (GTest `_deps/` and generated moc/qrc sources excluded).

Runner: `scripts/lint-cxx.sh` — clang-tidy on every project TU, clazy
(level0 + level1) on the Qt TUs (`apps/viewer/`, `tests/viewer/`). Exits
non-zero on any warning, so CI (Q3) can gate on it.

## Baseline (first run)

**152 clang-tidy warnings + 1 clazy warning.** After triage: 1 real bug
(BUG-27), ~30 warnings fixed, the rest silenced by calibrating the check list
(every disabled check is justified in `.clang-tidy`). Second run: **0 warnings**.

| Check | Count | Triage |
|---|---|---|
| `performance-enum-size` | 96 | **disabled** — packing enums into `uint8_t` buys nothing here and churns every declaration |
| `readability-isolate-declaration` | 11 | **disabled** — hits are semantic pairs (`const char *ts, *te` slice ranges in the parser; `int a, b` test fixtures) |
| `bugprone-multi-level-implicit-pointer-conversion` | 8 | **disabled** — all hits are `malloc`/`free`/`realloc` of pointer arrays, standard C allocation idiom |
| `readability-container-size-empty` | 5 | **fixed** — `filter(...).size() > 0` → `filter(...).isEmpty()` in buildArgs tests |
| `clang-analyzer-unix.Stream` | 5 | **fixed** — 1 real leak (BUG-27, see below); 4 GTest false positives in `test_emit_file.cpp` removed by holding `tmpfile()` in a RAII `FilePtr` (better anyway: closes on assertion early-return) |
| `clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling` | 4 | **disabled** — recommends Annex K `*_s` functions that macOS/glibc do not ship; all hits were plain `fprintf(stderr, ...)` |
| `readability-convert-member-functions-to-static` | 4 | **disabled** — every hit is a `Q_INVOKABLE` (ScenarioBridge) or a QuickTest hook; moc needs them as instance members |
| `readability-static-accessed-through-instance` | 3 | **fixed** — `app.setApplicationName(...)`/`app.exec()` → `QGuiApplication::...` in viewer `main.cpp` |
| `bugprone-unsafe-functions` (`rewind`) | 2 | **fixed** — `rewind` has no error reporting; replaced with checked `fseek(f, 0, SEEK_SET)` in `scenario_parse_file` and the `read_lines` test helper |
| `bugprone-branch-clone` | 2 | **fixed** — merged identical `case '?':`/`default:` (args.c) and `case PROC_DONE:`/`default:` (process.c) branches |
| `performance-implicit-conversion-in-loop` | 2 | **fixed** — `const QJsonValue &` bound to `QJsonValue(Const)Ref` copies per iteration; now `const auto &` |
| `readability-named-parameter` | 2 | **fixed** — unused params named as `/*error*/`, `/*ctx*/` |
| `clang-analyzer-unix.Errno` | 2 | **fixed** — both were downstream of the unchecked `rewind`; gone with the `fseek` fix |
| `bugprone-narrowing-conversions` | 1 | **fixed** — `int idx = QByteArray::indexOf(...)` → `qsizetype` in SimClient |
| `readability-redundant-casting` | 1 | **fixed** — dropped redundant `int(...)` around `QRandomGenerator::bounded(int)` |
| `readability-non-const-parameter` | 1 | **fixed** — `ensure_cap` reads `*count` only; now `const int *` |
| `readability-avoid-nested-conditional-operator` | 1 | **disabled** — cascaded chain (`a ? x : b ? y : z`) for device names reads fine, clang-format aligns it |
| `clang-analyzer-security.ArrayBound` (tainted index) | 1 | **false positive, NOLINT'd in place** — flagged `buf[n] = '\0'` in `scenario_parse_file`; `n = fread(...)` is bounded by `size` and `buf` is `malloc(size + 1)`. The analyzer taints every file-derived value (even fread's return) and cannot see the bound. Check stays on — it watches the untrusted-input parser |
| clazy `range-loop-detach` | 1 | **fixed** — range-for over the temporary `snap["events"].toArray()`; now iterates a `const QJsonArray` local |

Checks disabled up front while calibrating (before the first full run), also
justified in `.clang-tidy`: `bugprone-easily-swappable-parameters`,
`readability-identifier-length`, `readability-magic-numbers`,
`readability-braces-around-statements`, `readability-else-after-return`,
`readability-function-cognitive-complexity`,
`readability-uppercase-literal-suffix`,
`readability-math-missing-parentheses`,
`readability-redundant-access-specifiers` (Qt `slots:` sections),
plus `readability-implicit-bool-conversion` relaxed via
`AllowPointerConditions`/`AllowIntegerConditions` (`if (ptr)` is idiomatic in
the C lib and Qt code alike).

## Real bug found

**BUG-27** — `apps/rr-feedback/main.c`: when `sim_create` fails, the
`--emit-file` stream opened a few lines earlier was never closed. Error-path
resource leak; found by `clang-analyzer-unix.Stream`.

## How to run

```sh
scripts/lint-cxx.sh               # everything (tidy + clazy)
scripts/lint-cxx.sh --tidy-only src/scenario.c
scripts/lint-cxx.sh --clazy-only
```

Requires a configured debug preset (`cmake --preset debug`) for
`compile_commands.json`. Tools are found in `PATH` first, then in Qt
Creator's bundled LLVM.
