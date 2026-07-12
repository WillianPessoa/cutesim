# CuteSim

[![CI](https://github.com/WillianPessoa/cutesim/actions/workflows/ci.yml/badge.svg)](https://github.com/WillianPessoa/cutesim/actions/workflows/ci.yml)

Queue-based process scheduler simulator: a Round Robin with Feedback engine
written as a C11 library, driven by a CLI app and visualized live by a
Qt 6 / QML viewer.

## Layout

| Path | What it is |
|---|---|
| `src/`, `include/cutesim/` | simulation engine — C11 static library |
| `apps/rr-feedback/` | CLI simulator (trace, statistics, scenario files, TCP serve mode) |
| `apps/viewer/` | Qt 6 QML viewer (connects to `rr-feedback --serve`) |
| `apps/common/` | snapshot serializer + file/TCP emit backends |
| `scenarios/` | bundled preset scenarios shipped with the viewer |
| `examples/` | sample scenario files |
| `tests/` | GoogleTest + QML QuickTest suites (ctest) |

## Building

Requirements: CMake ≥ 3.25, a C11/C++20 toolchain, Qt ≥ 6.5 for the viewer.
Qt is located through `CMAKE_PREFIX_PATH` (set it in your environment if Qt
is not installed system-wide).

```sh
cmake --preset debug        # or: asan (ASan + UBSan)
cmake --build build
ctest --preset debug
```

To build only the C library and CLI on a machine without Qt:

```sh
cmake --preset debug -DCUTESIM_VIEWER=OFF
```

## Running

```sh
./build/apps/rr-feedback/rr-feedback --trace              # random workload
./build/apps/rr-feedback/rr-feedback scenarios/03-scripted-showcase.scn --trace
./build/apps/viewer/cutesim_viewer                        # launches rr-feedback itself
```

## Tooling

- `cmake --build build --target format` — clang-format + qmlformat in place
- `scripts/install-hooks.sh` — pre-commit format check
- `scripts/lint-cxx.sh` — clang-tidy over the project TUs (clazy opt-in)
