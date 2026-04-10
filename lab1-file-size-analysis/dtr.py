import os
import sys
import argparse

from core.settings.conf_reader import SETTINGS
from core.ansi.fix import enable_ansi_colors


def cmd_watch(args):
    """Run the original directory watcher."""
    path = os.path.abspath(args.path or SETTINGS.get("PATH", "."))

    if not os.path.isdir(path):
        print(f"Error: not a directory: {path}", file=sys.stderr)
        sys.exit(1)

    if os.name == "nt" and sys.stdout.isatty():
        enable_ansi_colors()

    SETTINGS["TARGET_DIR"] = path

    from core.dirtracker import watch
    watch(path, interactive=True)


def cmd_collect(args):
    """Collect file sizes from filesystem."""
    from core.collector import collect

    directory = args.path or SETTINGS.get("PATH", "/")
    data_dir = SETTINGS.get("DATA_DIR", "./data")
    output_path = os.path.join(data_dir, "result.txt")

    print(f"Collecting file sizes from: {directory}")
    collect(directory=directory, output_path=output_path)


def cmd_analyze(_args):
    """Analyze collected file size data."""
    from core.analyzer import analyze
    analyze()


def main():
    parser = argparse.ArgumentParser(
        description="dir-tracker: filesystem watcher and file size analyzer"
    )
    subparsers = parser.add_subparsers(dest="command")

    # watch
    watch_parser = subparsers.add_parser("watch", help="Watch directory for changes")
    watch_parser.add_argument("path", nargs="?", default=None, help="Directory to watch")

    # collect
    collect_parser = subparsers.add_parser("collect", help="Collect file sizes from filesystem")
    collect_parser.add_argument("path", nargs="?", default=None, help="Root directory to scan (default: /)")

    # analyze
    subparsers.add_parser("analyze", help="Analyze collected file size data")

    args = parser.parse_args()

    if args.command is None:
        parser.print_help()
        sys.exit(1)

    commands = {
        "watch": cmd_watch,
        "collect": cmd_collect,
        "analyze": cmd_analyze,
    }
    commands[args.command](args)


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as e:
        print(str(e))
