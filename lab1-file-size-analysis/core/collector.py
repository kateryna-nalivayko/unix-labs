"""Collector module: recursive filesystem traversal, streaming file sizes to disk."""

import os
import sys

from core.settings.conf_reader import SETTINGS
from core.settings.exclude_reader import exclude_dirs, exclude_files


def _walk_sizes(directory, excluded_dirs_set, excluded_files_set):
    """Generator that recursively yields file sizes (int) using os.scandir.

    Skips symlinks, permission errors, and OS errors silently.
    """
    try:
        with os.scandir(directory) as it:
            for entry in it:
                try:
                    if entry.is_symlink():
                        continue
                    if entry.is_dir(follow_symlinks=False):
                        if entry.name not in excluded_dirs_set:
                            yield from _walk_sizes(entry.path, excluded_dirs_set, excluded_files_set)
                    elif entry.is_file(follow_symlinks=False):
                        if entry.name not in excluded_files_set:
                            yield entry.stat(follow_symlinks=False).st_size
                except (PermissionError, OSError):
                    continue
    except (PermissionError, OSError):
        return


def collect(directory=None, output_path=None):
    """Collect file sizes from directory and write them to output_path.

    Returns the number of files collected.
    """
    if directory is None:
        directory = SETTINGS.get("PATH", "/")
    if output_path is None:
        output_path = os.path.join(SETTINGS.get("DATA_DIR", "./data"), "result.txt")

    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    excluded_dirs_set = set(exclude_dirs())
    excluded_files_set = set(exclude_files())

    count = 0
    with open(output_path, "w") as f:
        for size in _walk_sizes(directory, excluded_dirs_set, excluded_files_set):
            f.write(f"{size}\n")
            count += 1
            if count % 100000 == 0:
                print(f"  collected {count} files...", file=sys.stderr)

    print(f"Done: {count} files written to {output_path}", file=sys.stderr)
    return count
