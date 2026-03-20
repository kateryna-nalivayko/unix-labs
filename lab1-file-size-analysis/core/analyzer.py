"""Analyzer module: load data, compute statistics, find dominant range, print results."""

import os

import numpy as np

from core.settings.conf_reader import SETTINGS
from core.visualizer import plot_histogram, plot_cdf, plot_boxplot


BUCKETS = [
    (0, 1024, "0 - 1 KB"),
    (1024, 10 * 1024, "1 KB - 10 KB"),
    (10 * 1024, 100 * 1024, "10 KB - 100 KB"),
    (100 * 1024, 1024 ** 2, "100 KB - 1 MB"),
    (1024 ** 2, 10 * 1024 ** 2, "1 MB - 10 MB"),
    (10 * 1024 ** 2, 100 * 1024 ** 2, "10 MB - 100 MB"),
    (100 * 1024 ** 2, float("inf"), "100 MB+"),
]


def load_data(path):
    """Load file sizes from result.txt."""
    return np.loadtxt(path, dtype=np.int64)


def _fmt_size(n):
    """Format byte count as human-readable string."""
    if n < 1024:
        return f"{n} B"
    elif n < 1024 ** 2:
        return f"{n / 1024:.1f} KB"
    elif n < 1024 ** 3:
        return f"{n / 1024 ** 2:.1f} MB"
    else:
        return f"{n / 1024 ** 3:.2f} GB"


def compute_stats(sizes):
    """Compute statistical summary of file sizes."""
    percentiles = {p: np.percentile(sizes, p) for p in [50, 75, 90, 95, 99]}

    bucket_counts = []
    for lo, hi, label in BUCKETS:
        count = int(np.sum((sizes >= lo) & (sizes < hi)))
        bucket_counts.append((label, count, count / len(sizes) * 100))

    return {
        "count": len(sizes),
        "min": int(np.min(sizes)),
        "max": int(np.max(sizes)),
        "mean": float(np.mean(sizes)),
        "median": float(np.median(sizes)),
        "std": float(np.std(sizes)),
        "percentiles": percentiles,
        "buckets": bucket_counts,
    }


def find_dominant_range(sizes):
    """Find narrowest ranges containing >=75%, 80%, 85%, 90% of files.

    Uses symmetric percentile pairs (e.g., P5-P95 for 90%).
    Returns list of (percentage, low, high) tuples.
    """
    results = []
    for target_pct in [75, 80, 85, 90]:
        tail = (100 - target_pct) / 2
        lo = float(np.percentile(sizes, tail))
        hi = float(np.percentile(sizes, 100 - tail))
        results.append((target_pct, lo, hi))
    return results


def print_stats(stats, dominant_ranges):
    """Print formatted statistics and conclusions."""
    print("\n" + "=" * 60)
    print("  FILE SIZE DISTRIBUTION ANALYSIS")
    print("=" * 60)

    print(f"\n  Total files:  {stats['count']:,}")
    print(f"  Min size:     {_fmt_size(stats['min'])}")
    print(f"  Max size:     {_fmt_size(stats['max'])}")
    print(f"  Mean size:    {_fmt_size(int(stats['mean']))}")
    print(f"  Median size:  {_fmt_size(int(stats['median']))}")
    print(f"  Std dev:      {_fmt_size(int(stats['std']))}")

    print("\n  Percentiles:")
    for p, val in stats["percentiles"].items():
        print(f"    P{p:2d}: {_fmt_size(int(val))}")

    print("\n  Distribution by size range:")
    for label, count, pct in stats["buckets"]:
        bar = "#" * int(pct / 2)
        print(f"    {label:<16s}  {count:>10,}  ({pct:5.1f}%)  {bar}")

    print("\n  " + "-" * 56)
    print("  CONCLUSIONS:")
    for pct, lo, hi in dominant_ranges:
        print(f"    {pct}% of files have sizes from {_fmt_size(int(lo))} to {_fmt_size(int(hi))}")

    best = dominant_ranges[-1]
    print(f"\n  => The vast majority of files ({best[0]}%) have sizes")
    print(f"     in the range [{_fmt_size(int(best[1]))} .. {_fmt_size(int(best[2]))}]")
    print("=" * 60 + "\n")


def analyze(data_path=None, plots_dir=None):
    """Main analysis pipeline: load -> stats -> print -> visualize."""
    if data_path is None:
        data_path = os.path.join(SETTINGS.get("DATA_DIR", "./data"), "result.txt")
    if plots_dir is None:
        plots_dir = SETTINGS.get("PLOTS_DIR", "./plots")

    if not os.path.exists(data_path):
        print(f"Error: data file not found: {data_path}")
        print("Run 'python dtr.py collect' first.")
        return

    print(f"Loading data from {data_path}...")
    sizes = load_data(data_path)
    print(f"Loaded {len(sizes):,} file sizes.")

    stats = compute_stats(sizes)
    dominant_ranges = find_dominant_range(sizes)
    print_stats(stats, dominant_ranges)

    os.makedirs(plots_dir, exist_ok=True)
    print("Generating plots...")
    plot_histogram(sizes, os.path.join(plots_dir, "histogram.png"))
    plot_cdf(sizes, os.path.join(plots_dir, "cdf.png"))
    plot_boxplot(sizes, os.path.join(plots_dir, "boxplot.png"))
    print("Done.")
