from invoke import task

@task
def type_check(c):
    """Run mypy for static type checking."""
    c.run("mypy .")

@task
def format(c):
    """Run ruff for code formatting."""
    c.run("RUFF_CACHE_DIR=cache/.ruff_cache ruff check . --fix")

@task
def format_check(c):
    """Check for formatting issues using ruff."""
    c.run("RUFF_CACHE_DIR=cache/.ruff_cache ruff check .")
    