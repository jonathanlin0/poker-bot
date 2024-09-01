from invoke import task
import shutil
import os

@task
def type_check(c):
    """Run mypy for static type checking."""
    c.run("mypy .")

@task
def format(c):
    """Format code using black and enforce linting rules using ruff."""
    # Format code with black
    c.run("black --line-length 20 .")
    # Run ruff to check and apply any remaining formatting rules
    c.run("ruff check . --fix")

@task
def format_check(c):
    """Check for formatting issues using ruff."""
    c.run("RUFF_CACHE_DIR=cache/.ruff_cache ruff check .")
    
@task
def clear_cache(c):
    """Delete all __pycache__ directories recursively."""
    for root, dirs, files in os.walk('.'):
        for dir_name in dirs:
            if dir_name == '__pycache__':
                cache_path = os.path.join(root, dir_name)
                shutil.rmtree(cache_path)
                print(f"Deleted: {cache_path}")
                