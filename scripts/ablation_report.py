#!/usr/bin/env python3
"""
ablation_report.py -- turns results_ablation.csv (produced by running
bench_pre_opt / bench_stage1 / bench_stage2 / bench_current against the
same file) into a per-stage marginal comparison table.

This reads directly from the CSV benchmark_runner.cpp already writes --
nothing here is hardcoded. Rerun any of the four binaries as many times
as you want (e.g. to redo a noisy run); this script always reflects
whatever is currently in the CSV.

Usage:
    python3 scripts/ablation_report.py results_ablation.csv
    python3 scripts/ablation_report.py results_ablation.csv --out ablation_summary.csv
"""

import csv
import sys
import argparse
from collections import defaultdict

# The four build labels, in stage order, exactly as benchmark_runner.cpp
# prints them. If you ever rename a label in benchmark_runner.cpp, update
# it here too -- these strings are the only place this script needs to
# know about your specific build labels.
STAGE_ORDER = [
    "pre-optimization (in progress)",
    "stage1 (change1 only)",
    "stage2 (change1+2)",
    "post-optimization (current)",
]

STAGE_SHORT = {
    "pre-optimization (in progress)": "pre_opt",
    "stage1 (change1 only)": "stage1",
    "stage2 (change1+2)": "stage2",
    "post-optimization (current)": "current",
}

# Only these phases are "query phase" timings -- parse-phase rows
# (loadFile, indexStructure, constructTree, total_parse) are skipped,
# since this report is about query-executor changes specifically.
QUERY_PHASES = [
    "wildcard fan-out",
    "filter: numeric WHERE",
    "filter: AND",
    "filter: OR",
    "filter: long-string WHERE",
]


def pct_faster(before, after):
    """% faster going from `before` to `after`. Positive = got faster."""
    if before == 0:
        return 0.0
    return (before - after) / before * 100


def load_csv(path):
    """
    Returns: data[size_mb][build_label][phase] = time_ms (float)

    If a (size, build, phase) combo appears more than once in the CSV
    (e.g. you reran a noisy binary), the LAST occurrence wins -- so
    rerunning a single stage and re-appending to the same CSV correctly
    overrides the earlier noisy row without needing to edit the CSV by
    hand.
    """
    data = defaultdict(lambda: defaultdict(dict))
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            size = row["size_mb"]
            build = row["build"]
            phase = row["phase"]
            if phase not in QUERY_PHASES:
                continue
            try:
                time_ms = float(row["time_ms"])
            except ValueError:
                continue
            data[size][build][phase] = time_ms
    return data


def build_report(data):
    """
    Returns a list of row dicts, one per (size, query), with absolute
    timings for all four stages plus marginal % for each individual
    change and the total % vs. pre_opt.
    """
    rows = []
    # sort sizes numerically (they're strings like "25.8136")
    for size in sorted(data.keys(), key=lambda s: float(s)):
        builds = data[size]
        missing_stages = [s for s in STAGE_ORDER if s not in builds]
        if missing_stages:
            print(
                f"[skip] size={size}MB missing stages: {missing_stages} "
                f"-- run all four binaries against this file first.",
                file=sys.stderr,
            )
            continue

        for phase in QUERY_PHASES:
            try:
                pre_opt = builds[STAGE_ORDER[0]][phase]
                stage1 = builds[STAGE_ORDER[1]][phase]
                stage2 = builds[STAGE_ORDER[2]][phase]
                current = builds[STAGE_ORDER[3]][phase]
            except KeyError:
                print(f"[skip] size={size}MB phase={phase!r}: missing a data point", file=sys.stderr)
                continue

            rows.append({
                "size_mb": round(float(size), 1),
                "query": phase,
                "pre_opt_ms": round(pre_opt, 3),
                "stage1_ms": round(stage1, 3),
                "stage2_ms": round(stage2, 3),
                "current_ms": round(current, 3),
                "change1_marginal_pct": round(pct_faster(pre_opt, stage1), 1),
                "change2_marginal_pct": round(pct_faster(stage1, stage2), 1),
                "change3_marginal_pct": round(pct_faster(stage2, current), 1),
                "total_vs_preopt_pct": round(pct_faster(pre_opt, current), 1),
            })
    return rows


def flag_noisy_controls(rows, threshold=15.0):
    """
    The 'wildcard fan-out' query touches no WHERE-clause code at all --
    it runs identical code on every stage. Any large swing on it isn't
    a real effect, it's system noise from that particular run. Anything
    above `threshold`% on this row is worth flagging before trusting
    the other numbers from the same (size, stage) run.
    """
    warnings = []
    for r in rows:
        if r["query"] != "wildcard fan-out":
            continue
        for label, pct in [
            ("change1", r["change1_marginal_pct"]),
            ("change2", r["change2_marginal_pct"]),
            ("change3", r["change3_marginal_pct"]),
        ]:
            if abs(pct) > threshold:
                warnings.append(
                    f"  {r['size_mb']}MB control moved {pct:+.1f}% at {label} -- "
                    f"this stage's run is likely noisy, consider rerunning it."
                )
    return warnings


def print_table(rows):
    header = f"{'size':>6} | {'query':<24} | {'pre_opt':>8} | {'stage1':>8} | {'stage2':>8} | {'current':>8} | {'d1':>6} | {'d2':>6} | {'d3':>6} | {'total':>6}"
    print(header)
    print("-" * len(header))
    for r in rows:
        print(
            f"{r['size_mb']:>6} | {r['query']:<24} | "
            f"{r['pre_opt_ms']:>8} | {r['stage1_ms']:>8} | {r['stage2_ms']:>8} | {r['current_ms']:>8} | "
            f"{r['change1_marginal_pct']:>+6.1f} | {r['change2_marginal_pct']:>+6.1f} | "
            f"{r['change3_marginal_pct']:>+6.1f} | {r['total_vs_preopt_pct']:>+6.1f}"
        )


def write_csv(rows, out_path):
    if not rows:
        print("No rows to write.", file=sys.stderr)
        return
    with open(out_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)
    print(f"\nWrote {len(rows)} rows to {out_path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_path", help="path to results_ablation.csv")
    parser.add_argument("--out", default=None, help="optional: write summary table to this CSV path")
    args = parser.parse_args()

    data = load_csv(args.csv_path)
    if not data:
        print(f"No usable rows found in {args.csv_path}", file=sys.stderr)
        sys.exit(1)

    rows = build_report(data)
    if not rows:
        print("No complete (size, query) rows across all four stages -- nothing to report.", file=sys.stderr)
        sys.exit(1)

    print_table(rows)

    warnings = flag_noisy_controls(rows)
    if warnings:
        print("\n[!] Noise warnings (control query moved more than expected):")
        for w in warnings:
            print(w)

    if args.out:
        write_csv(rows, args.out)


if __name__ == "__main__":
    main()