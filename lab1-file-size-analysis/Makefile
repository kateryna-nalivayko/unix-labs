.PHONY: collect analyze clean

collect:
	uv run python dtr.py collect

analyze:
	uv run python dtr.py analyze

clean:
	uv run python -c "import shutil; shutil.rmtree('data', True); shutil.rmtree('plots', True)"
