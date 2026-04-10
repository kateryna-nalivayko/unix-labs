# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Educational Unix systems lab series. Python 3.14+, managed with **uv** package manager. Documentation and reports are in Ukrainian.

Currently contains **Lab 1: File Size Distribution Analysis** — a pipeline that collects file sizes from a filesystem, computes statistics, and generates visualizations.

## Build & Run Commands

All commands run from `lab1-file-size-analysis/`:

```bash
make collect          # Scan filesystem, write sizes to data/result.txt
make analyze          # Compute stats and generate plots from collected data
make clean            # Remove data/ and plots/ directories
```

Equivalent manual commands:

```bash
uv run python dtr.py collect [path]   # path defaults to current directory
uv run python dtr.py analyze
uv run python dtr.py watch [path]     # directory change detection
```

Install dependencies: `uv sync`

On Windows, `make` requires Git Bash or similar. Alternatively use the `uv run python dtr.py` commands directly.

## Architecture

Each lab lives in its own top-level directory (e.g., `lab1-file-size-analysis/`).

### Lab 1 Pipeline

```
collector (os.scandir) → data/result.txt → analyzer (numpy) → visualizer (matplotlib) → plots/
```

- **`dtr.py`** — CLI entry point with `collect`, `analyze`, `watch` subcommands
- **`core/collector.py`** — Generator-based recursive filesystem traversal, streams one file size per line to `data/result.txt`
- **`core/analyzer.py`** — Loads sizes with numpy, computes percentiles/buckets/dominant ranges, calls visualizer
- **`core/visualizer.py`** — Matplotlib Agg backend, generates histogram (log-scale), CDF, and boxplot PNGs to `plots/`
- **`core/dirtracker.py`** — Snapshot-based directory change detection using MD5 hashes and pickle persistence
- **`core/settings/`** — Reads `settings/config.conf` (key=value) and exclusion lists at import time
- **`core/ansi/`** — ANSI color output with Windows compatibility via ctypes/kernel32
- **`generate_report.py`** — Produces `.docx` report with embedded stats and plots (Ukrainian text, Times New Roman)

### Configuration

Settings are in `lab1-file-size-analysis/settings/`:
- `config.conf` — scan path, data/plots directories, paths to exclusion files
- `excluded_dirs.conf` / `excluded_files.conf` — exclusion lists (supports `#` comments)

## Conventions

- Functional design, no classes — modules expose standalone functions
- Private helpers prefixed with `_` (e.g., `_walk_sizes()`, `_fmt_size()`)
- Errors handled gracefully: `PermissionError`/`OSError` caught during traversal, interactive prompts for critical failures
- Cross-platform: Windows ANSI support, `os.path` for paths
- Runtime artifacts (`data/`, `plots/`, `*.pkl`) are gitignored

## Commit Rules

- Do NOT add a `Co-Authored-By` line to commit messages.
