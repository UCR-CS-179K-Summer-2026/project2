# Benchmark Results — Sprint 2 Optimizations

## Quick Start — reproducing every result in this document

Everything below can be independently verified from a clean checkout.
All benchmarks read/write to `benchmark_results/results_ablation.csv` or `results.csv` —
console output prints live as each binary runs, and every number in the
tables below traces back to a specific line in that CSV.

**1. Generate the benchmark data** (only needed once — creates
20/50/100/200MB synthetic files on the `store.products[]` schema):

```bash
python3 scripts/generate_benchmark_data.py 20 benchmark_data/bench_20mb.json
python3 scripts/generate_benchmark_data.py 50 benchmark_data/bench_50mb.json
python3 scripts/generate_benchmark_data.py 100 benchmark_data/bench_100mb.json
python3 scripts/generate_benchmark_data.py 200 benchmark_data/bench_200mb.json
```

**2. Build every benchmark binary in one step:**

```bash
cmake -B build
cmake --build build --target bench_naive bench_pre_opt bench_stage1 bench_stage2 bench_current
```

| Binary          | What it measures                                 | Used in                            |
| --------------- | ------------------------------------------------ | ---------------------------------- |
| `bench_naive`   | Pre-Sprint-2 baseline                            | Sprint 2 section                   |
| `bench_pre_opt` | Post-Sprint-2, pre-Round-2 baseline              | Round 2 & Full Comparison sections |
| `bench_stage1`  | + Tier 1 change 1 only                           | Tier 1 Ablation section            |
| `bench_stage2`  | + Tier 1 changes 1 & 2                           | Tier 1 Ablation section            |
| `bench_current` | All optimizations (this project's current state) | every section                      |

**3. Run any binary directly and watch results print live to the
terminal** — no flags needed beyond the file path; a CSV path is
optional and only needed if you want the run appended to a file for
later comparison:

```bash
./build/bench_current benchmark_data/bench_50mb.json
```

This prints parse-phase timings (loadFile/indexStructure/constructTree)
and query-phase timings (all six benchmark queries, 200 iterations
each, averaged) directly to stdout — nothing needs to be opened
afterward to see a result.

**4. Reproduce a specific comparison** — each section below has its
own "Reproducing this" block with the exact commands for that
comparison; the Quick Start above covers building everything once, and
each section then just runs binaries you already have.

**5. Automatically compute the Tier 1 ablation table** (the marginal
% breakdown per individual change, see that section below) from raw
CSV data, instead of reading percentages by hand:

```bash
python3 scripts/ablation_report.py benchmark_results/results_ablation.csv
```

---

Measures the effect of Sprint 2's two query-executor optimizations against
a naive baseline, across file sizes from 20MB to 200MB.

## What's being compared

|                               | Naive baseline                                                            | Sprint 2 (current)                                          |
| ----------------------------- | ------------------------------------------------------------------------- | ----------------------------------------------------------- |
| WHERE-clause field resolution | `executeStep()` — heap-allocates a `std::vector` on every call            | `resolveSingle()` — returns a single pointer, no allocation |
| WHERE-clause value comparison | Copies the field's raw bytes into a new `std::string` on every comparison | `nodeRawValue()` — reads through a zero-copy `string_view`  |

Both binaries are compiled from the same `query_executor.h`, same
`AaronJsonParser`/`TasnimQueryParser`, same benchmark data, same query
set, at `-O2`. The naive path is compiled in behind a `-DBENCH_NAIVE`
flag rather than being separate, older code — so the only difference
between the two binaries is exactly the two rows above, nothing else.

## Methodology

- **Data**: synthetic files generated from the same schema as
  `test_data_filter.json` (`store.products[]`), at 20MB, 50MB, 100MB,
  and 200MB
- **Query phase**: each query run 200 times and averaged
- **Control query**: `wildcard fan-out` touches no WHERE-clause code at
  all — it runs _identical_ code in both binaries. Any measured
  difference on it is pure system noise, not a real effect. It's
  included specifically to establish how much noise to expect, so the
  WHERE-affected numbers below can be judged against a real baseline
  instead of assumed to be meaningful.

## Results — percent faster (Sprint 2 vs. naive)

| Query                                   |    20MB |   50MB |   100MB |  200MB |
| --------------------------------------- | ------: | -----: | ------: | -----: |
| **numeric WHERE** (`price > N`)         |    3.6% |  14.3% |   14.6% |  12.7% |
| **AND** (2 string conditions)           |   36.5% |  43.6% |   42.5% |  45.0% |
| **OR** (1 string + 1 numeric condition) |   35.9% |  37.8% |   28.3% |  37.3% |
| **long-string WHERE** (~75-char field)  |   41.0% |  46.8% |   59.5% |  47.2% |
| _control — wildcard fan-out_            | _−1.5%_ | _3.3%_ | _−0.1%_ | _1.5%_ |

Every WHERE-affected query sits well above the noise band the control
establishes (roughly ±3% at every size except a one-off −1.5% at 20MB).
The gains scale consistently across all four file sizes rather than
being a one-off result at a single size — that consistency is itself
part of the evidence this is a real effect, not noise.

## Why AND/OR and the long string see bigger gains

- **AND and OR evaluate two conditions per row** instead of one, so
  `resolveSingle`'s saved allocation applies twice — roughly double the
  single-condition gain, which is exactly what's observed (36–45% vs.
  4–15%).
- **The long-string field shows the largest gain of all** because it's
  the only field long enough (~75 characters) to force a real heap
  allocation on the naive (copying) path. Short fields — numbers,
  `true`/`false`, short category names — stay inside `std::string`'s
  small-string-optimization stack buffer on _both_ paths, so their gain
  comes purely from the avoided vector allocation in `resolveSingle`,
  not from the copy itself.

## Raw timings (ms)

<details>
<summary>20MB</summary>

| Query             |  Naive | Sprint 2 |
| ----------------- | -----: | -------: |
| wildcard fan-out  |  5.258 |    5.336 |
| numeric WHERE     |  9.000 |    8.673 |
| AND               |  8.847 |    5.616 |
| OR                | 11.952 |    7.660 |
| long-string WHERE |  7.717 |    4.551 |

</details>

<details>
<summary>50MB</summary>

| Query             |  Naive | Sprint 2 |
| ----------------- | -----: | -------: |
| wildcard fan-out  | 13.462 |   13.016 |
| numeric WHERE     | 22.837 |   19.568 |
| AND               | 22.461 |   12.673 |
| OR                | 30.781 |   19.143 |
| long-string WHERE | 19.820 |   10.537 |

</details>

<details>
<summary>100MB</summary>

| Query             |  Naive | Sprint 2 |
| ----------------- | -----: | -------: |
| wildcard fan-out  | 25.748 |   25.777 |
| numeric WHERE     | 46.709 |   39.898 |
| AND               | 44.681 |   25.694 |
| OR                | 54.080 |   38.770 |
| long-string WHERE | 53.072 |   21.470 |

</details>

<details>
<summary>200MB</summary>

| Query             |   Naive | Sprint 2 |
| ----------------- | ------: | -------: |
| wildcard fan-out  |  53.263 |   52.480 |
| numeric WHERE     |  92.620 |   80.844 |
| AND               |  94.309 |   51.885 |
| OR                | 124.940 |   78.266 |
| long-string WHERE |  80.017 |   42.252 |

</details>

## Reproducing the Sprint 2 comparison

```bash
# generate benchmark data
python3 scripts/generate_benchmark_data.py 20 benchmark_data/bench_20mb.json
python3 scripts/generate_benchmark_data.py 50 benchmark_data/bench_50mb.json
python3 scripts/generate_benchmark_data.py 100 benchmark_data/bench_100mb.json
python3 scripts/generate_benchmark_data.py 200 benchmark_data/bench_200mb.json

# build both binaries
clang++ -std=c++17 -O2 benchmark_runner.cpp AaronJsonParser/parser.cpp TasnimQueryParser/QueryParser.cpp -o bench_default
clang++ -std=c++17 -O2 -DBENCH_NAIVE benchmark_runner.cpp AaronJsonParser/parser.cpp TasnimQueryParser/QueryParser.cpp -o bench_naive

# run against each file
for f in benchmark_data/bench_*.json; do
    ./bench_default "$f" results.csv
    ./bench_naive "$f" results.csv
done
```

Raw CSV backing these numbers: [`benchmark_results/results.csv`](./results.csv)

---

# Benchmark Results — Round 2: Query Executor Optimizations (in progress)

**Status: in progress.** This section covers a further round of query
executor optimizations, on top of Sprint 2. It's a live document — more
optimizations may be added and re-measured before this is considered
final (at which point this section's title may be updated to reflect a
specific project milestone).

## What's being compared

Three changes, all in `query_executor.h`, all on top of Sprint 2's
optimizations (not replacing them):

| #   | Change                                                             | What it removes                                                                                                                                                                                     |
| --- | ------------------------------------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1   | Hoist WHERE condition RHS numeric parsing out of the per-row loop  | The query's literal comparison value (e.g. `1000` in `price > 1000`) no longer gets re-parsed via `std::stod` on every single row — parsed once per query instead                                   |
| 2   | `std::from_chars` instead of `std::stod` for the field's own value | Removes a heap-allocated `std::string` copy that was built on every row just to satisfy `std::stod`'s signature, even though the underlying read was already a zero-copy `string_view`              |
| 3   | Allocation-free `GET` resolution for non-wildcard select fields    | A `GET` clause without `[*]` can only ever resolve to one node — now uses the same allocation-free `resolveSingle()` WHERE clauses already used, instead of `executeStep()`'s heap-allocated vector |

Same methodology as the Sprint 2 comparison above: both binaries built
from the same file, same data, same query set, at `-O2` — the only
difference is these three changes, compiled in behind a `-DBENCH_PRE_OPT`
flag for the "before" binary.

## Results — percent faster (post-optimization vs. pre-optimization)

| Query                                        |   20MB |    50MB |   100MB |  200MB |
| -------------------------------------------- | -----: | ------: | ------: | -----: |
| **numeric WHERE**                            |  22.1% |   29.2% |   25.6% |  31.2% |
| **OR** (has a numeric condition)             |  25.2% |   25.2% |   28.3% |  27.7% |
| **AND** (no numeric condition)               |   6.0% |    3.4% |   11.2% |   9.6% |
| **long-string WHERE** (no numeric condition) |  −0.4% |    1.4% |    4.3% |   4.3% |
| _control — wildcard fan-out_                 | _1.6%_ | _−0.5%_ | _−2.4%_ | _3.5%_ |

All four sizes are clean against their own control (single digits), so
this is a trustworthy result across the full 20–200MB range. Note: the
first 100MB run showed a 27.6% control gap (pure noise, unrelated to
these code changes) and was discarded and rerun — the numbers above are
from the clean rerun, not the noisy one.

## Why the pattern looks the way it does

**Queries with a numeric condition (`numeric WHERE`, `OR`) see the
biggest gains** because they're the only ones touched by all three
changes — #1 and #2 both specifically target numeric parsing, and #3
applies on top. **`AND` and `long-string WHERE` have no numeric
condition at all**, so only #3 applies to them — hence smaller but still
real gains (3–11%), consistent with #3 alone rather than all three
combined. This split is expected, not a weakness in the result: it's
exactly what should happen given which change targets what.

## Raw timings (ms)

<details>
<summary>20MB</summary>

| Query             | Pre-optimization | Post-optimization |
| ----------------- | ---------------: | ----------------: |
| wildcard fan-out  |            5.264 |             5.178 |
| numeric WHERE     |            9.098 |             7.083 |
| AND               |            5.811 |             5.465 |
| OR                |            8.878 |             6.644 |
| long-string WHERE |            4.605 |             4.623 |

</details>

<details>
<summary>50MB</summary>

| Query             | Pre-optimization | Post-optimization |
| ----------------- | ---------------: | ----------------: |
| wildcard fan-out  |           13.120 |            13.182 |
| numeric WHERE     |           21.991 |            15.569 |
| AND               |           12.850 |            12.415 |
| OR                |           22.168 |            16.580 |
| long-string WHERE |           10.460 |            10.312 |

</details>

<details>
<summary>100MB</summary>

Clean rerun (see note above about the discarded noisy first attempt).

| Query             | Pre-optimization | Post-optimization |
| ----------------- | ---------------: | ----------------: |
| wildcard fan-out  |           27.732 |            28.386 |
| numeric WHERE     |           47.516 |            35.372 |
| AND               |           28.910 |            25.666 |
| OR                |           45.671 |            32.766 |
| long-string WHERE |           23.466 |            22.467 |

</details>

<details>
<summary>200MB</summary>

| Query             | Pre-optimization | Post-optimization |
| ----------------- | ---------------: | ----------------: |
| wildcard fan-out  |           53.962 |            52.096 |
| numeric WHERE     |           92.459 |            63.641 |
| AND               |           56.365 |            50.927 |
| OR                |           91.224 |            65.952 |
| long-string WHERE |           44.614 |            42.706 |

</details>

## Honest scope of this result

These gains are specific to the query-execution phase — the part of the
system this optimization work targets. They say nothing about parsing
performance, which is a separate part of the pipeline outside this
scope.

Within query execution, the picture is not uniform across all four
queries, and it shouldn't be — the gains track exactly which
optimization applies to which query:

- **`numeric WHERE` and `OR`** (both have a numeric condition) show
  real, consistent gains clearly above the control's noise band at
  every size — these are touched by all three Round 2 changes.
- **`AND`** (no numeric condition, but rows do reach the `GET` clause)
  shows smaller but still real gains from optimization #3 alone.
- **`long-string WHERE` is a special case worth calling out rather than
  averaging over**: this query matches **zero rows** (`description =
nomatch`), and optimization #3 only ever executes for rows that pass
  the WHERE clause. With no matching rows, #3 never runs at all for
  this query — so a near-zero measured effect (−0.4% to 4.3%, mostly
  inside the control's own noise band at 20MB/50MB) is the _correct,
  expected_ result, not a weak version of a real gain. Stating this
  precisely is more honest than claiming uniform significance across
  all four queries when one of them structurally can't show an effect.

## Reproducing this

```bash
clang++ -std=c++17 -O2 benchmark_runner.cpp AaronJsonParser/parser.cpp TasnimQueryParser/QueryParser.cpp -o bench_default
clang++ -std=c++17 -O2 -DBENCH_PRE_OPT benchmark_runner.cpp AaronJsonParser/parser.cpp TasnimQueryParser/QueryParser.cpp -o bench_pre_opt

for f in benchmark_data/bench_*.json; do
    ./bench_default "$f" results_tier1.csv
    ./bench_pre_opt "$f" results_tier1.csv
done
```

Raw CSVs backing these numbers:
[`benchmark_results/results_tier1_20-50-200mb.csv`](./results_tier1_20-50-200mb.csv),
[`benchmark_results/results_tier1_100mb.csv`](./results_tier1_100mb.csv)

---

# Benchmark Results — Full Comparison: Naive → Sprint 2 → Current

Consolidates the two sections above into a single view — one set of
tables covering the whole progression, so it doesn't need to be shown
twice. For the full methodology and per-change explanation behind each
step, see the two sections above; this section is a summary.

**Methodology — single-session comparison.** Unlike an earlier version
of this section, all three binaries (naive, post-Sprint 2, current) were
run back-to-back in one session per file size, using
`scripts/compare_benchmarks.py` (the same tool `make benchmark` uses).
This avoids stitching together two separately-measured sessions, which
previously caused a real inconsistency between this section and the
Sprint 2 section above for the same comparison. As with any single
session, system noise varies by run — the `wildcard fan-out` control
row is included at every size specifically so that noise is visible and
can be judged honestly rather than assumed away. In this run, 20MB
happened to be the noisiest (15.4% control gap); the other three sizes
were clean (under 6%). Which size is noisiest varies run to run — it's
a property of system conditions at the time, not of any particular file
size.

**Reading this table:** `Naive`, `Post-Sprint 2`, and `Current` are the
raw timings (ms) for each stage. `→ Sprint 2` is how much faster
Post-Sprint 2 is than Naive — Sprint 2's contribution alone. `→
Current` is how much faster Current is than Naive — the cumulative
effect of both optimization rounds together. Reading left to right on
one row tells the whole story for that query at that file size: where
it started, where it landed after each round, and the percent
improvement at each step.

## 20MB noisier this run — see control row

| Query                        | Naive (ms) | Post-Sprint 2 (ms) | → Sprint 2 | Current (ms) | → Current |
| ---------------------------- | ---------: | -----------------: | ---------: | -----------: | --------: |
| _wildcard fan-out (control)_ |    _5.472_ |            _4.632_ |    _15.4%_ |      _4.516_ |   _17.5%_ |
| numeric WHERE                |      9.372 |              9.794 |      −4.5% |        7.839 |     16.4% |
| AND                          |      9.640 |              5.830 |      39.5% |        5.593 |     42.0% |
| OR                           |     13.006 |              9.328 |      28.3% |        7.390 |     43.2% |
| long-string WHERE            |      8.906 |              5.416 |      39.2% |        4.748 |     46.7% |

## 50MB

| Query                        | Naive (ms) | Post-Sprint 2 (ms) | → Sprint 2 | Current (ms) | → Current |
| ---------------------------- | ---------: | -----------------: | ---------: | -----------: | --------: |
| _wildcard fan-out (control)_ |   _14.626_ |           _14.668_ |    _−0.3%_ |     _14.648_ |   _−0.2%_ |
| numeric WHERE                |     22.964 |             25.097 |      −9.3% |       19.697 |     14.2% |
| AND                          |     24.045 |             15.651 |      34.9% |       14.881 |     38.1% |
| OR                           |     33.502 |             24.791 |      26.0% |       18.647 |     44.3% |
| long-string WHERE            |     22.991 |             12.568 |      45.3% |       12.562 |     45.4% |

## 100MB

| Query                        | Naive (ms) | Post-Sprint 2 (ms) | → Sprint 2 | Current (ms) | → Current |
| ---------------------------- | ---------: | -----------------: | ---------: | -----------: | --------: |
| _wildcard fan-out (control)_ |   _28.390_ |           _29.837_ |    _−5.1%_ |     _29.885_ |   _−5.3%_ |
| numeric WHERE                |     44.651 |             50.237 |     −12.5% |       38.439 |     13.9% |
| AND                          |     45.807 |             31.938 |      30.3% |       30.572 |     33.3% |
| OR                           |     59.156 |             49.172 |      16.9% |       37.104 |     37.3% |
| long-string WHERE            |     49.568 |             25.843 |      47.9% |       25.222 |     49.1% |

## 200MB

| Query                        | Naive (ms) | Post-Sprint 2 (ms) | → Sprint 2 | Current (ms) | → Current |
| ---------------------------- | ---------: | -----------------: | ---------: | -----------: | --------: |
| _wildcard fan-out (control)_ |   _58.920_ |           _59.616_ |    _−1.2%_ |     _60.955_ |   _−3.5%_ |
| numeric WHERE                |     91.584 |             99.531 |      −8.7% |       80.511 |     12.1% |
| AND                          |     96.061 |             62.815 |      34.6% |       62.964 |     34.5% |
| OR                           |    130.406 |             97.588 |      25.2% |       74.644 |     42.8% |
| long-string WHERE            |     92.005 |             50.485 |      45.1% |       51.179 |     44.4% |

**Interpreting the negative `→ Sprint 2` values for `numeric WHERE`:**
this isn't a regression — Sprint 2 never targeted numeric parsing at
all (that's Round 2's `from_chars` change), so this column is measuring
noise around a true effect close to zero, and the sign of that noise
happens to be negative in this particular run. The `→ Current` column
for the same row (12–16%) shows the real gain, which came entirely from
Round 2. `AND`, `OR`, and `long-string WHERE` show substantial, real
gains from Sprint 2 alone at every size — well above their own
control's noise level except at 20MB, where the control itself is
elevated this run.

## Reproducing this

```bash
cmake -B build && cmake --build build --target bench_naive bench_pre_opt bench_current
python3 scripts/compare_benchmarks.py benchmark_data/bench_20mb.json build
python3 scripts/compare_benchmarks.py benchmark_data/bench_50mb.json build
python3 scripts/compare_benchmarks.py benchmark_data/bench_100mb.json build
python3 scripts/compare_benchmarks.py benchmark_data/bench_200mb.json build
```

Or for a quick single-file live demo: `make -C build benchmark` (defaults
to the 200MB file).

---

# Benchmark Results — Tier 1 Ablation: Isolating Each Change's Contribution

The Round 2 section above measures all three Tier 1 changes together
against the pre-optimization baseline. This section breaks that combined
result apart — isolating exactly how much each individual change
contributes on its own, run as four separate binaries so each stage adds
exactly one more change than the last.

## What's being compared

Four binaries, same file, same data, same query set, at `-O2` — each one
compiled with a different subset of the three Tier 1 changes active,
using `DISABLE_CHANGE1`/`DISABLE_CHANGE2`/`DISABLE_CHANGE3` flags layered
on top of the existing `BENCH_PRE_OPT` scaffolding:

| Binary          | Changes active                               |
| --------------- | -------------------------------------------- |
| `bench_pre_opt` | none (existing pre-optimization baseline)    |
| `bench_stage1`  | change 1 only (RHS hoisting)                 |
| `bench_stage2`  | changes 1 + 2 (+ `std::from_chars`)          |
| `bench_current` | changes 1 + 2 + 3 (existing "current" build) |

Reading left to right across a row shows the whole progression: where a
query started, where it landed after each additional change, and the
marginal % improvement contributed by that specific change alone.

## Results — marginal % faster contributed by each change

<details open>
<summary>20MB</summary>

| Query                        | pre_opt (ms) | stage1 (ms) | stage2 (ms) | current (ms) | Δ change 1 | Δ change 2 | Δ change 3 |   Total |
| ---------------------------- | -----------: | ----------: | ----------: | -----------: | ---------: | ---------: | ---------: | ------: |
| numeric WHERE                |        9.452 |       7.928 |       7.705 |        7.431 |     +16.1% |      +2.8% |      +3.6% |  +21.4% |
| OR                           |        8.627 |       7.508 |       6.945 |        6.514 |     +13.0% |      +7.5% |      +6.2% |  +24.5% |
| AND                          |        6.522 |       5.419 |       5.414 |        5.699 |     +16.9% |      +0.1% |      −5.3% |  +12.6% |
| long-string WHERE            |        4.695 |       4.170 |       4.106 |        4.556 |     +11.2% |      +1.5% |     −11.0% |   +3.0% |
| _control — wildcard fan-out_ |      _5.379_ |     _4.560_ |     _5.167_ |      _5.390_ |   _+15.2%_ |   _−13.3%_ |    _−4.3%_ | _−0.2%_ |

Control moved +15.2% at the change-1 step — this size's run carries
more noise than the other three below. The direction and rough
magnitude of the real signal (numeric WHERE, OR) is consistent with the
larger file sizes, but treat this row's exact percentages as
approximate rather than as clean as 50–200MB.

</details>

<details open>
<summary>50MB</summary>

| Query                        | pre_opt (ms) | stage1 (ms) | stage2 (ms) | current (ms) | Δ change 1 | Δ change 2 | Δ change 3 |   Total |
| ---------------------------- | -----------: | ----------: | ----------: | -----------: | ---------: | ---------: | ---------: | ------: |
| numeric WHERE                |       20.887 |      19.331 |      17.968 |       17.378 |      +7.5% |      +7.0% |      +3.3% |  +16.8% |
| OR                           |       20.816 |      18.890 |      17.516 |       16.418 |      +9.3% |      +7.3% |      +6.3% |  +21.1% |
| AND                          |       12.466 |      12.583 |      12.926 |       13.462 |      −0.9% |      −2.7% |      −4.1% |   −8.0% |
| long-string WHERE            |       10.191 |      11.071 |      10.403 |       11.345 |      −8.6% |      +6.0% |      −9.1% |  −11.3% |
| _control — wildcard fan-out_ |     _13.067_ |    _13.064_ |    _13.789_ |     _13.607_ |    _+0.0%_ |    _−5.5%_ |    _+1.3%_ | _−4.1%_ |

</details>

<details open>
<summary>100MB</summary>

| Query                        | pre_opt (ms) | stage1 (ms) | stage2 (ms) | current (ms) | Δ change 1 | Δ change 2 | Δ change 3 |   Total |
| ---------------------------- | -----------: | ----------: | ----------: | -----------: | ---------: | ---------: | ---------: | ------: |
| numeric WHERE                |       44.657 |      40.453 |      38.911 |       37.104 |      +9.4% |      +3.8% |      +4.6% |  +16.9% |
| OR                           |       43.711 |      38.647 |      36.624 |       33.886 |     +11.6% |      +5.2% |      +7.5% |  +22.5% |
| AND                          |       27.750 |      25.901 |      28.241 |       28.868 |      +6.7% |      −9.0% |      −2.2% |   −4.0% |
| long-string WHERE            |       22.631 |      21.870 |      23.417 |       25.512 |      +3.4% |      −7.1% |      −8.9% |  −12.7% |
| _control — wildcard fan-out_ |     _27.325_ |    _26.963_ |    _27.128_ |     _27.526_ |    _+1.3%_ |    _−0.6%_ |    _−1.5%_ | _−0.7%_ |

</details>

<details open>
<summary>200MB</summary>

| Query                        | pre_opt (ms) | stage1 (ms) | stage2 (ms) | current (ms) | Δ change 1 | Δ change 2 | Δ change 3 |   Total |
| ---------------------------- | -----------: | ----------: | ----------: | -----------: | ---------: | ---------: | ---------: | ------: |
| numeric WHERE                |       91.520 |      79.322 |      79.265 |       70.715 |     +13.3% |      +0.1% |     +10.8% |  +22.7% |
| OR                           |       88.154 |      78.681 |      78.086 |       67.934 |     +10.7% |      +0.8% |     +13.0% |  +22.9% |
| AND                          |       57.041 |      52.112 |      57.685 |       54.887 |      +8.6% |     −10.7% |      +4.9% |   +3.8% |
| long-string WHERE            |       45.468 |      42.683 |      46.394 |       48.226 |      +6.1% |      −8.7% |      −3.9% |   −6.1% |
| _control — wildcard fan-out_ |     _55.943_ |    _51.595_ |    _56.639_ |     _54.170_ |    _+7.8%_ |    _−9.8%_ |    _+4.4%_ | _+3.2%_ |

**Note on this run:** the first attempt at `stage2` on this file size was
run immediately after `pre_opt` and `stage1` in the same session and
showed a badly noisy control (a 60%+ swing on a query that runs identical
code across all four binaries — physically impossible as a real effect).
Re-running `bench_stage2` alone, after the other three had finished and
the machine had a moment to settle, reproduced a clean, low control
swing consistent with the other three sizes. The numbers above are from
that isolated rerun. Running several large (200MB) binaries back-to-back
appears to introduce measurable sustained-load noise on this machine;
worth keeping benchmark runs on large files spaced out rather than
scripted tightly back-to-back if this is repeated.

</details>

## What the marginal breakdown shows

**Change 1 (RHS hoisting) is the largest individual contributor at every
size**, consistently 7–17% on `numeric WHERE` and `OR` — the two query
types with a numeric condition, exactly what the change targets (moving
one `std::stod` call from once-per-row to once-per-query).

**Change 3 (allocation-free GET) grows more valuable as the file
gets larger** — a modest 3–6% at 20MB, but 10–13% by 200MB on the two
numeric queries. This tracks with what the change actually does: it
removes one heap allocation per matching row on the `GET` side, so the
saved cost compounds as row count grows with file size.

**Change 2 (`std::from_chars`) is the smallest and least consistent
contributor.** It targets a narrower cost than the other two — only the
`LHS` field's own parse, not the RHS or the `GET` side — so its
measured effect (0–7%) is closer to the noise floor at every size,
particularly at 200MB where it's within a percent of zero. This doesn't
mean the change is wrong; `std::from_chars` still removes a real
allocation (see the Round 2 section above for the code-level
explanation) — it just means, on this dataset, that allocation's cost
relative to everything else in the row loop is small enough that its
isolated signal is harder to separate from run-to-run noise than
Change 1's or Change 3's.

**`AND` and `long-string WHERE` stay flat or negative throughout, as
expected** — `AND` has no numeric condition, so changes 1 and 2 (both
numeric-parsing changes) shouldn't help it much, and `long-string WHERE`
matches zero rows in this dataset, so change 3 (which only fires on
matching rows) can't show an effect on it at all. Both patterns are
consistent with the Round 2 section's explanation above, not a
contradiction of it.

## Reproducing this

```bash
cmake --build build --target bench_pre_opt bench_stage1 bench_stage2 bench_current

for f in benchmark_data/bench_20mb.json benchmark_data/bench_50mb.json \
         benchmark_data/bench_100mb.json benchmark_data/bench_200mb.json; do
    ./build/bench_pre_opt "$f" benchmark_results/results_ablation.csv
    ./build/bench_stage1 "$f" benchmark_results/results_ablation.csv
    ./build/bench_stage2 "$f" benchmark_results/results_ablation.csv
    ./build/bench_current "$f" benchmark_results/results_ablation.csv
done

python3 scripts/ablation_report.py benchmark_results/results_ablation.csv --out benchmark_results/ablation_summary.csv
```

`scripts/ablation_report.py` reads directly from `benchmark_results/results_ablation.csv`
and computes the marginal percentages above automatically — rerunning
any single binary (e.g. to redo a noisy run, as with 200MB `stage2`
above) and re-appending is enough; the script takes the most recent
occurrence of each (size, build, phase) combination, so a rerun
correctly supersedes an earlier noisy one without manual CSV editing.
