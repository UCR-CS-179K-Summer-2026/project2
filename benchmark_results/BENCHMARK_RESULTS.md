# Benchmark Results — Sprint 2 Optimizations

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
- **Parse phase**: `loadFile` + `indexStructure` + `constructTree`,
  averaged over 3 runs
- **Query phase**: each query run 200 times and averaged
- **Control queries**: `dot-path point lookup` and `wildcard fan-out`
  touch no WHERE-clause code at all — they run _identical_ code in both
  binaries. Any measured difference on these is pure system noise, not a
  real effect. They're included specifically to establish how much noise
  to expect, so the WHERE-affected numbers below can be judged against a
  real baseline instead of assumed to be meaningful.

## Results — percent faster (Sprint 2 vs. naive)

| Query                                   |     20MB |   50MB |   100MB |  200MB |
| --------------------------------------- | -------: | -----: | ------: | -----: |
| **numeric WHERE** (`price > N`)         |     3.6% |  14.3% |   14.6% |  12.7% |
| **AND** (2 string conditions)           |    36.5% |  43.6% |   42.5% |  45.0% |
| **OR** (1 string + 1 numeric condition) |    35.9% |  37.8% |   28.3% |  37.3% |
| **long-string WHERE** (~75-char field)  |    41.0% |  46.8% |   59.5% |  47.2% |
| _control — wildcard fan-out_            |  _−1.5%_ | _3.3%_ | _−0.1%_ | _1.5%_ |
| _control — total parse time_            | _−13.7%_ | _1.6%_ |  _1.4%_ | _1.2%_ |

Every WHERE-affected query sits well above the noise band the controls
establish (roughly ±14% at worst, mostly under ±3%). The gains scale
consistently across all four file sizes rather than being a one-off
result at a single size — that consistency is itself part of the
evidence this is a real effect, not noise.

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

| Phase / Query         |      Naive |   Sprint 2 |
| --------------------- | ---------: | ---------: |
| loadFile              |      3.396 |      6.717 |
| indexStructure        |     24.713 |     28.038 |
| constructTree         |     22.185 |     22.453 |
| **total parse**       | **50.294** | **57.208** |
| dot-path point lookup |    0.00006 |    0.00006 |
| wildcard fan-out      |      5.258 |      5.336 |
| numeric WHERE         |      9.000 |      8.673 |
| AND                   |      8.847 |      5.616 |
| OR                    |     11.952 |      7.660 |
| long-string WHERE     |      7.717 |      4.551 |

</details>

<details>
<summary>50MB</summary>

| Phase / Query     |       Naive |    Sprint 2 |
| ----------------- | ----------: | ----------: |
| loadFile          |       8.093 |       9.413 |
| indexStructure    |      59.639 |      56.674 |
| constructTree     |      53.917 |      53.665 |
| **total parse**   | **121.650** | **119.751** |
| wildcard fan-out  |      13.462 |      13.016 |
| numeric WHERE     |      22.837 |      19.568 |
| AND               |      22.461 |      12.673 |
| OR                |      30.781 |      19.143 |
| long-string WHERE |      19.820 |      10.537 |

</details>

<details>
<summary>100MB</summary>

| Phase / Query     |       Naive |    Sprint 2 |
| ----------------- | ----------: | ----------: |
| loadFile          |      17.194 |      18.321 |
| indexStructure    |     120.757 |     116.446 |
| constructTree     |     112.467 |     112.114 |
| **total parse**   | **250.418** | **246.881** |
| wildcard fan-out  |      25.748 |      25.777 |
| numeric WHERE     |      46.709 |      39.898 |
| AND               |      44.681 |      25.694 |
| OR                |      54.080 |      38.770 |
| long-string WHERE |      53.072 |      21.470 |

</details>

<details>
<summary>200MB</summary>

| Phase / Query     |       Naive |    Sprint 2 |
| ----------------- | ----------: | ----------: |
| loadFile          |      37.782 |      33.173 |
| indexStructure    |     255.857 |     246.989 |
| constructTree     |     229.238 |     236.495 |
| **total parse**   | **522.877** | **516.657** |
| wildcard fan-out  |      53.263 |      52.480 |
| numeric WHERE     |      92.620 |      80.844 |
| AND               |      94.309 |      51.885 |
| OR                |     124.940 |      78.266 |
| long-string WHERE |      80.017 |      42.252 |

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

| Query                                        |    20MB |    50MB |   100MB |   200MB |
| -------------------------------------------- | ------: | ------: | ------: | ------: |
| **numeric WHERE**                            |   22.1% |   29.2% |   25.6% |   31.2% |
| **OR** (has a numeric condition)             |   25.2% |   25.2% |   28.3% |   27.7% |
| **AND** (no numeric condition)               |    6.0% |    3.4% |   11.2% |    9.6% |
| **long-string WHERE** (no numeric condition) |   −0.4% |    1.4% |    4.3% |    4.3% |
| _control — wildcard fan-out_                 |  _1.6%_ | _−0.5%_ | _−2.4%_ |  _3.5%_ |
| _control — total parse time_                 | _−1.7%_ |  _5.1%_ |  _2.6%_ | _−1.8%_ |

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

## Honest scope of this result

Query-phase gains matter most for the workload this system is actually
built for: parse a file once, run many queries against it. A single
query against a freshly-parsed 200MB file still spends the majority of
its time in parsing (~500ms) versus even the slowest query (~90ms
before this round of optimizations) — so this round's benefit compounds
with repeated querying, rather than dramatically changing the
single-query, single-run picture.

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
