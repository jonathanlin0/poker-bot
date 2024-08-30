from invoke import task

@task
def type_check(c):
    """Run mypy for static type checking."""
    c.run("mypy .")
