# 🔍 JSON Analytics Engine

**A high-performance, C++17 engine for querying large JSON files — built for speed, not just correctness.**

**Team:** Aaron · Poojan · Tasnim
**Documentation:** [Full architecture, algorithms, benchmarks & query reference →](#) <ucr-cs-179k-summer-2026.github.io/project2/>

---

## Table of Contents

- [Overview](#-overview)
- [Highlights](#-highlights)
- [Quick Start](#-quick-start)
- [Running Tests](#-running-tests)
- [Benchmarking](#-benchmarking)
- [System Requirements](#-system-requirements)
- [Project Structure](#-project-structure)
- [Query Syntax at a Glance](#-query-syntax-at-a-glance)

---

## Overview

This project takes a **JSON file** and a **query**, and returns exactly the data that matches — no external database, no third-party JSON library. Two query forms are supported: fast **dot-path** lookups (`store.products[0].name`) and richer **GET/FROM/WHERE** filter queries (`GET name FROM store.products WHERE price > 300`).

The engineering focus isn't just "does it work" — it's **does it stay fast as input grows**. Every optimization in this codebase is backed by a reproducible benchmark, and every regression caught during development is documented, not hidden.

---

## Highlights

| Component | What it does |
|---|---|
| **SIMD JSON Parser** (`AaronSimd`) | Optional AVX2-accelerated parser, scans 32 bytes at a time (x86 only) |
| **Primary JSON Parser** (`AaronJsonParser`) | Cross-platform, runs everywhere including Apple Silicon |
| **Dot-Path Queries** | Array indexing, `[*]` wildcards, and bracket-quoted keys for keys plain syntax can't express |
| **Filter Queries** | `GET/FROM/WHERE` with `AND` / `OR` / `NOT`, correct precedence grouping, short-circuit evaluation |
| **Zero-Allocation Execution Path** | Non-wildcard field resolution and hoisted numeric comparisons avoid heap allocation entirely |
| **53 GoogleTest Cases** | Covers traversal, filtering, escape-sequence decoding, and tree-structure verification |
| **Full Benchmark Suite** | Naive vs. optimized comparisons across 20MB–200MB files, with per-optimization ablation |

---

## Quick Start

### 1. Clone

```bash
git clone <repository-url>
cd <repository-name>
```

### 2. Build

Requires **CMake 3.14+** and a **C++17 compiler**. First build fetches GoogleTest automatically via `FetchContent` (needs internet once, cached after).

```bash
cmake -B build
cmake --build build
```

### 3. Run

```bash
./build/test_executor
```

### 4. Load a file and query it

```
Enter JSON file name (or type QUIT / HELP): test_data/test_data_filter.json
File loaded successfully.

Enter a query (or type HELP / QUIT): GET name FROM store.products WHERE price > 300
```

Type `HELP` → option `1` at any point for full query syntax, operators, and examples. After each query, the menu lets you load a different file, run another query, or quit.

---

## Running Tests

```bash
cmake --build build --target engine_tests
ctest --test-dir build
```

Expect **53/53 tests passed**.

A `Makefile` wrapper is included so you don't need to remember the CMake invocations:

```bash
make test        # build + run all tests
make coverage     # build + test + generate coverage report (requires gcovr)
```

---

## Benchmarking

All commands below are also documented — with full explanations — on the [project website](#). <!-- replace # with your gh-pages URL -->

**1. Generate synthetic benchmark data** (one-time; seeded, so output is byte-identical every run):

```bash
python3 scripts/generate_benchmark_data.py 20  benchmark_data/bench_20mb.json
python3 scripts/generate_benchmark_data.py 50  benchmark_data/bench_50mb.json
python3 scripts/generate_benchmark_data.py 100 benchmark_data/bench_100mb.json
python3 scripts/generate_benchmark_data.py 200 benchmark_data/bench_200mb.json
```

**2. Build the benchmark binaries:**

```bash
cmake --build build --target bench_naive bench_pre_opt bench_stage1 bench_stage2 bench_current
```

| Binary | Measures |
|---|---|
| `bench_naive` | Pre-Sprint-2 baseline |
| `bench_pre_opt` | Post-Sprint-2, pre-Sprint-4 baseline |
| `bench_stage1` | + Change 1 only (RHS hoisting) |
| `bench_stage2` | + Changes 1 & 2 (`std::from_chars`) |
| `bench_current` | All optimizations — current state |

**3. Run any binary directly:**

```bash
./build/bench_current benchmark_data/bench_50mb.json
```

**4. Reproduce the full Naive → Sprint 2 → Current comparison:**

```bash
python3 scripts/compare_benchmarks.py benchmark_data/bench_20mb.json  build
python3 scripts/compare_benchmarks.py benchmark_data/bench_50mb.json  build
python3 scripts/compare_benchmarks.py benchmark_data/bench_100mb.json build
python3 scripts/compare_benchmarks.py benchmark_data/bench_200mb.json build
```

Or for a single quick live-demo run: `make -C build benchmark` (defaults to the 200MB file).

**5. Reproduce the per-optimization ablation:**

```bash
for f in benchmark_data/bench_20mb.json benchmark_data/bench_50mb.json \
         benchmark_data/bench_100mb.json benchmark_data/bench_200mb.json; do
    ./build/bench_pre_opt "$f" benchmark_results/results_ablation.csv
    ./build/bench_stage1  "$f" benchmark_results/results_ablation.csv
    ./build/bench_stage2  "$f" benchmark_results/results_ablation.csv
    ./build/bench_current "$f" benchmark_results/results_ablation.csv
done

python3 scripts/ablation_report.py benchmark_results/results_ablation.csv \
    --out benchmark_results/ablation_summary.csv
```

Full explanation of every column and how to read the ablation table is on the documentation site.

---

## System Requirements

### Software

| Requirement | Notes |
|---|---|
| **CMake** | 3.14 or newer |
| **C++17 compiler** | Tested on Apple Clang (macOS) and GCC (Linux); MSVC untested |
| **Python 3** | Only for benchmark scripts in `scripts/` — not required to build/run the core engine |
| **gcovr** *(optional)* | Only for `make coverage` — `pip install gcovr --break-system-packages` |
| **Internet access** | Required on first build only, to fetch GoogleTest via `FetchContent` |

### Hardware

| Component | Requirement |
|---|---|
| **Default build** (`AaronJsonParser`) | Any platform, any CPU architecture — including Apple Silicon (ARM64) |
| **Optional SIMD build** (`AaronSimd`) | **x86 CPU with AVX2 support** required; not part of the default build, not portable to ARM |
| **RAM** | Scales with input file size — the engine loads the full JSON tree into memory (no streaming), so benchmarking at 200MB comfortably needs a few hundred MB free; smaller test files need negligible memory |
| **Disk** | Benchmark data files (20–200MB) are generated locally and `.gitignore`d, not stored in the repo — budget ~400MB free disk if generating all four sizes at once |

---

## Project Structure

```
├── AaronJsonParser/     # Primary, cross-platform JSON parser
├── AaronSimd/           # Optional AVX2-accelerated JSON parser (x86 only)
├── TasnimQueryParser/   # Query string parser + CLI
├── QueryExecutor/       # Query execution engine and tree verifier
├── benchmark_results/   # Benchmark output CSVs and reports
├── scripts/             # Benchmark data generation, comparison, and ablation scripts
├── test_data/           # JSON fixture files used by the test suite
├── tests/               # GoogleTest suite
├── main.cpp             # CLI entry point
└── CMakeLists.txt
```

---

## Query Syntax at a Glance

```
# Dot-path — direct lookup
store.products[0].name
store.products[*].["product name"]     # bracket-quoted key, for keys with spaces/dots

# Filter query — GET/FROM/WHERE
GET name FROM store.products
GET name FROM store.products WHERE price > 300
GET name FROM store.products WHERE category = electronics AND inStock = true OR category = furniture
GET name FROM store.products WHERE NOT inStock = true
```

Full syntax reference, edge-case behavior (missing keys, malformed input, empty arrays), and more examples are documented on the [project website](#). <ucr-cs-179k-summer-2026.github.io/project2/>
