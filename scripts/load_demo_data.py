import os
import sys
import urllib.request

BASE_URL = "https://jonathanlin.net/media/public/poker/prod_demo_files"
DATA_DIR = "data"
CHUNK_SIZE = 8192
BAR_WIDTH = 40

FILES = [
    "avg_strat.bin",
    "precomputed_equity.txt",
]


def format_bytes(n):
    for unit in ("B", "KB", "MB", "GB"):
        if n < 1024:
            return f"{n:.1f} {unit}"
        n /= 1024
    return f"{n:.1f} TB"


def download(url, dest):
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    with urllib.request.urlopen(req) as resp, open(dest, "wb") as f:
        total = int(resp.headers.get("Content-Length", 0))
        downloaded = 0
        while True:
            chunk = resp.read(CHUNK_SIZE)
            if not chunk:
                break
            f.write(chunk)
            downloaded += len(chunk)
            if total:
                pct = downloaded / total
                filled = int(BAR_WIDTH * pct)
                bar = "█" * filled + "░" * (BAR_WIDTH - filled)
                line = f"\r  [{bar}] {pct:6.1%}  {format_bytes(downloaded)} / {format_bytes(total)}"
                sys.stdout.write(line.ljust(80))
            else:
                sys.stdout.write(f"\r  {format_bytes(downloaded)} downloaded".ljust(80))
            sys.stdout.flush()
    print()


os.makedirs(DATA_DIR, exist_ok=True)

for filename in FILES:
    dest = os.path.join(DATA_DIR, filename)
    if os.path.exists(dest):
        print(f"{dest} already exists, skipping")
        continue
    url = f"{BASE_URL}/{filename}"
    print(f"Downloading {filename}...")
    download(url, dest)
