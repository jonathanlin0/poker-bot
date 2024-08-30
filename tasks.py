from invoke import task

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
    