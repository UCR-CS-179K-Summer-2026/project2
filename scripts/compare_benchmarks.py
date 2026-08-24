#!/usr/bin/env python3
"""
Runs bench_naive, bench_pre_opt, and bench_current against the same file
and prints one merged side-by-side table -- for live-demo use, so the
professor/TA sees one clear comparison instead of three separate walls
of text to explain individually.

Usage:
    python3 scripts/compare_benchmarks.py <path-to-json-file> [binary-dir]

binary-dir defaults to "build" (where CMake puts the three bench_* binaries
by default). If you built them somewhere else, pass that path as the
second argument.
"""
import csv
import os
import subprocess
import sys
import tempfile

# Only these rows get shown -- matches the project's own scope: query
# execution performance, not the parser's parse-phase timing.
QUERY_ROWS = [
    "wildcard fan-out",
    "filter: numeric WHERE",
    "filter: AND",
    "filter: OR",
    "filter: long-string WHERE",
]

BINARIES = [
    ("bench_naive", "Naive"),
    ("bench_pre_opt", "Post-Sprint 2"),
    ("bench_current", "Current"),
]


def run_and_parse(binary_path: str, json_file: str) -> dict:
    """Runs one benchmark binary, returns {row_label: time_ms}."""
    tmp = tempfile.NamedTemporaryFile(suffix=".csv", delete=False)
    tmp.close()
    os.unlink(tmp.name)  # benchmark_runner checks file existence to decide
    csv_path = tmp.name  # whether to write a CSV header -- must not pre-exist

    result = subprocess.run(
        [binary_path, json_file, csv_path],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"ERROR running {binary_path}:\n{result.stderr}", file=sys.stderr)
        sys.exit(1)

    values = {}
    with open(csv_path) as f:
        for row in csv.DictReader(f):
            if row["phase"] in QUERY_ROWS:
                values[row["phase"]] = float(row["time_ms"])
    os.unlink(csv_path)
    return values


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    json_file = sys.argv[1]
    binary_dir = sys.argv[2] if len(sys.argv) >= 3 else "build"

    if not os.path.isfile(json_file):
        print(f"File not found: {json_file}")
        sys.exit(1)

    file_size_mb = os.path.getsize(json_file) / (1024 * 1024)
    print(f"\n=== Benchmark comparison: {json_file} ({file_size_mb:.1f} MB) ===\n")

    results = {}
    for binary_name, label in BINARIES:
        binary_path = os.path.join(binary_dir, binary_name)
        if not os.path.isfile(binary_path):
            print(f"Binary not found: {binary_path}\n"
                  f"Build it first with: cmake --build {binary_dir} --target {binary_name}")
            sys.exit(1)
        print(f"Running {label}...")
        results[label] = run_and_parse(binary_path, json_file)

    # Print merged table
    labels = [label for _, label in BINARIES]
    col_width = 16
    header = "Query".ljust(28) + "".join(l.rjust(col_width) for l in labels)
    print("\n" + header)
    print("-" * len(header))
    for row in QUERY_ROWS:
        display_name = row.replace("filter: ", "")
        line = display_name.ljust(28)
        for label in labels:
            val = results[label].get(row)
            line += (f"{val:.3f} ms".rjust(col_width) if val is not None else "N/A".rjust(col_width))
        print(line)
    print()


if __name__ == "__main__":
    main()