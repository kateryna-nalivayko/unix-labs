"""Visualizer module: histogram, CDF, and boxplot of file size distributions."""

import os

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker


def _size_label(x, _pos=None):
    """Format byte size as human-readable label."""
    if x < 1024:
        return f"{x:.0f} B"
    elif x < 1024 ** 2:
        return f"{x / 1024:.0f} KB"
    elif x < 1024 ** 3:
        return f"{x / 1024 ** 2:.0f} MB"
    else:
        return f"{x / 1024 ** 3:.1f} GB"


def plot_histogram(sizes, output_path):
    """Plot a histogram of file sizes with log-scale X axis."""
    if len(sizes) == 0:
        return

    positive = sizes[sizes > 0]
    if len(positive) == 0:
        return

    fig, ax = plt.subplots(figsize=(12, 6))

    bins = np.logspace(np.log10(positive.min()), np.log10(positive.max()), 50)
    ax.hist(positive, bins=bins, edgecolor="black", linewidth=0.3, color="#4C72B0")

    ax.set_xscale("log")
    ax.xaxis.set_major_formatter(ticker.FuncFormatter(_size_label))
    ax.set_xlabel("File size")
    ax.set_ylabel("Number of files")
    ax.set_title("File Size Distribution (Histogram)")
    ax.grid(True, alpha=0.3)
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close(fig)
    print(f"  Saved: {output_path}")


def plot_cdf(sizes, output_path):
    """Plot cumulative distribution function of file sizes."""
    if len(sizes) == 0:
        return

    positive = np.sort(sizes[sizes > 0])
    if len(positive) == 0:
        return

    cdf = np.arange(1, len(positive) + 1) / len(positive) * 100

    fig, ax = plt.subplots(figsize=(12, 6))
    ax.plot(positive, cdf, linewidth=1.5, color="#4C72B0")

    for pct in [50, 75, 90, 95]:
        ax.axhline(y=pct, color="gray", linestyle="--", alpha=0.5, linewidth=0.8)
        val = np.percentile(positive, pct)
        ax.axvline(x=val, color="red", linestyle=":", alpha=0.4, linewidth=0.8)
        ax.annotate(f"P{pct}: {_size_label(val)}", xy=(val, pct),
                    fontsize=8, color="red", ha="left", va="bottom")

    ax.set_xscale("log")
    ax.xaxis.set_major_formatter(ticker.FuncFormatter(_size_label))
    ax.set_xlabel("File size")
    ax.set_ylabel("Cumulative % of files")
    ax.set_title("Cumulative Distribution Function (CDF)")
    ax.grid(True, alpha=0.3)
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close(fig)
    print(f"  Saved: {output_path}")


def plot_boxplot(sizes, output_path):
    """Plot boxplot of file sizes on log scale."""
    if len(sizes) == 0:
        return

    positive = sizes[sizes > 0]
    if len(positive) == 0:
        return

    log_sizes = np.log10(positive.astype(np.float64))

    fig, ax = plt.subplots(figsize=(10, 4))
    bp = ax.boxplot(log_sizes, vert=False, widths=0.6, patch_artist=True,
                    boxprops=dict(facecolor="#4C72B0", alpha=0.7),
                    medianprops=dict(color="red", linewidth=2))

    ticks = np.arange(0, np.ceil(log_sizes.max()) + 1)
    ax.set_xticks(ticks)
    ax.set_xticklabels([_size_label(10 ** t) for t in ticks], rotation=45)
    ax.set_xlabel("File size (log scale)")
    ax.set_title("File Size Distribution (Boxplot)")
    ax.grid(True, alpha=0.3, axis="x")
    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close(fig)
    print(f"  Saved: {output_path}")
