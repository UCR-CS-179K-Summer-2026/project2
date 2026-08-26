#!/usr/bin/env python3
"""
Generates large JSON files matching the same store.products schema used by
test_data_filter.json, at target sizes, for benchmarking parse/query
performance. Separate from correctness test data ( these are used only for
timing, never for asserting expected query results.)

Usage: python3 generate_benchmark_data.py <target_mb> <output_path>
Example: python3 generate_benchmark_data.py 20 bench_20mb.json

[NEW] Generation is seeded (see random.seed() call below), so re-running
this script with the same <target_mb> produces byte-identical output every
time.
"""
import json
import random
import sys

CATEGORIES = ["electronics", "furniture", "appliances", "office", "outdoor"]
ADJECTIVES = ["Wireless", "Compact", "Pro", "Deluxe", "Standard", "Portable", "Heavy-Duty"]
NOUNS = ["Mouse", "Keyboard", "Monitor", "Desk", "Chair", "Lamp", "Speaker", "Charger", "Cable", "Stand"]
MAKERS = ["Acme", "Globex", "Initech", "Umbrella", "Hooli", "Stark"]

DESCRIPTION_PARTS = [
    "Engineered for durability and everyday reliability in demanding environments",
    "Designed with premium materials for a comfortable long-term ownership experience",
    "Combines modern styling with dependable performance across a wide range of uses",
    "Built to exceed industry standards for quality, safety, and consistent output",
    "Optimized for efficiency while maintaining a compact and lightweight footprint",
]

# [NEW] Fixed seed so generation is fully deterministic. This is set once at import time (rather than inside generate()) so that estimate_record_bytes() which also draws from the RNG to build its 200-record sample -- doesn't perturb the sequence generate() itself relies on; re-seeding right before the main record loop guarantees the actual output records are identical
SEED = 42

def make_product(i):
    name = f"{random.choice(ADJECTIVES)} {random.choice(NOUNS)} {i}"
    product = {
        "name": name,
        "price": round(random.uniform(5, 2000), 2),
        "inStock": random.choice([True, False]),
        "category": random.choice(CATEGORIES),
        "description": random.choice(DESCRIPTION_PARTS),
    }
    # roughly 1 in 5 products get a nested maker object, same asymmetry pattern as test_data_filter.json (only the Laptop entry has "maker")
    if i % 5 == 0:
        product["maker"] = {
            "name": f"{random.choice(MAKERS)} {random.choice(NOUNS)}s",
            "location": random.choice(["USA", "China", "Germany", "Vietnam"]),
        }
    return product

def estimate_record_bytes():
    sample = [make_product(i) for i in range(200)]
    sample_json = json.dumps(sample)
    return len(sample_json) / len(sample)

def generate(target_mb, output_path):
    target_bytes = target_mb * 1024 * 1024

    # [NEW] Re-seed immediately before estimation so the whole generation process -- estimate pass + main write loop -- is deterministic from a single known starting point, not just the main loop in isolation.
    random.seed(SEED)
    avg_record_bytes = estimate_record_bytes()

    # [NEW] Re-seed again before the real record loop.
    random.seed(SEED)

    # +2 for the outer {"store":{"name":...,"products":[ ]}} wrapper overhead
    n_records = int(target_bytes / avg_record_bytes)

    print(f"Target: {target_mb}MB, estimated record size: {avg_record_bytes:.1f}B, "
          f"generating ~{n_records} products (seed={SEED})...")

    import os
    out_dir = os.path.dirname(output_path)
    if out_dir and not os.path.isdir(out_dir):
        os.makedirs(out_dir, exist_ok=True)
        print(f"Created directory: {out_dir}")

    with open(output_path, "w") as f:
        f.write('{"store":{"name":"BenchMart","products":[')
        for i in range(n_records):
            if i > 0:
                f.write(",")
            json.dump(make_product(i), f, separators=(",", ":"))
        f.write("]}}")

    import os
    actual_mb = os.path.getsize(output_path) / (1024 * 1024)
    print(f"Wrote {output_path}: {actual_mb:.1f}MB, {n_records} products")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 generate_benchmark_data.py <target_mb> <output_path>")
        sys.exit(1)
    generate(float(sys.argv[1]), sys.argv[2])