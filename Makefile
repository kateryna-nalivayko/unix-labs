.PHONY: collect analyze clean

collect:
	cd lab1-file-size-analysis && uv run python dtr.py collect

analyze:
	cd lab1-file-size-analysis && uv run python dtr.py analyze

clean:
	cd lab1-file-size-analysis && uv run python -c "import shutil; shutil.rmtree('data', True); shutil.rmtree('plots', True)"
