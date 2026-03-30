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


def get_remote_size(url):
    req = urllib.request.Request(url, method="HEAD", headers={"User-Agent": "Mozilla/5.0"})
    try:
        with urllib.request.urlopen(req) as resp:
            return int(resp.headers.get("Content-Length", 0))
    except Exception:
        return 0


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
    url = f"{BASE_URL}/{filename}"

    if os.path.exists(dest):
        local_size = os.path.getsize(dest)
        remote_size = get_remote_size(url)
        if not remote_size:
            print(f"{filename}: could not check remote size, keeping local copy")
            continue
        if local_size != remote_size:
            print(f"{filename} changed ({format_bytes(local_size)} -> {format_bytes(remote_size)}), re-downloading...")
            os.remove(dest)
        else:
            print(f"{dest} is up to date, skipping")
            continue

    print(f"Downloading {filename}...")
    download(url, dest)
