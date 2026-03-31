import os
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
import numpy as np

DATA_DIR = "data"

STREET_COLORS = {
    0: "#e74c3c",
    1: "#e67e22",
    2: "#2ecc71",
    3: "#3498db",
}
STREET_NAMES = {0: "Preflop", 1: "Flop", 2: "Turn", 3: "River"}

NUM_MARKERS = 15


def get_num_threads(experiment_path):
    metadata_path = os.path.join(experiment_path, "metadata.txt")
    if not os.path.exists(metadata_path):
        return None
    with open(metadata_path, "r") as f:
        for line in f:
            key, _, value = line.strip().partition(":")
            if key == "num-threads":
                return int(value)
    return None


def load_infoset_data():
    """Returns dict: experiment_name -> {entries, num_threads}"""
    results = {}

    for experiment_name in sorted(os.listdir(DATA_DIR)):
        experiment_path = os.path.join(DATA_DIR, experiment_name)
        if not os.path.isdir(experiment_path):
            continue

        txt_path = os.path.join(experiment_path, "infosets_wrt_epoch.txt")
        if not os.path.exists(txt_path):
            continue

        try:
            with open(txt_path, "r") as f:
                lines = [l.strip() for l in f.readlines()]

            entries = []
            i = 0
            while i < len(lines):
                if lines[i] == "":
                    i += 1
                    continue
                epoch = int(lines[i])
                counts = [int(lines[i + 1 + s]) for s in range(4)]
                entries.append((epoch, counts))
                i += 5

            if entries:
                num_threads = get_num_threads(experiment_path)
                results[experiment_name] = {
                    "entries": entries,
                    "num_threads": num_threads,
                }
        except Exception as e:
            print(f"Error loading {txt_path}: {e}")

    return results


def evenly_spaced_indices(epochs, n):
    """Pick n indices so that the chosen epochs are roughly evenly spaced."""
    if len(epochs) <= n:
        return list(range(len(epochs)))
    targets = np.linspace(epochs[0], epochs[-1], n)
    epochs_arr = np.array(epochs)
    return sorted(set(np.argmin(np.abs(epochs_arr[:, None] - targets[None, :]), axis=0)))


def plot_infosets(ax, infoset_data):
    legend_added = set()

    for experiment_name, info in infoset_data.items():
        entries = info["entries"]
        is_multi = info["num_threads"] is not None and info["num_threads"] > 1
        marker = "*" if is_multi else None
        markersize = 12 if is_multi else None
        epochs = [e[0] for e in entries]
        marker_indices = evenly_spaced_indices(epochs, NUM_MARKERS) if is_multi else None
        for street in range(4):
            counts = [e[1][street] for e in entries]
            color = STREET_COLORS[street]
            label = STREET_NAMES[street] if street not in legend_added else None
            legend_added.add(street)
            ax.plot(epochs, counts, color=color, linewidth=1.5, alpha=0.85,
                    marker=marker, markersize=markersize,
                    markevery=marker_indices, label=label)

    street_handles = [
        Line2D([0], [0], color=STREET_COLORS[s], linewidth=1.5, label=STREET_NAMES[s])
        for s in range(4)
    ]
    thread_handles = [
        Line2D([0], [0], color="black", linewidth=1.5, label="Single-threaded"),
        Line2D([0], [0], color="black", linewidth=1.5, marker="*", markersize=18, label="Multi-threaded"),
    ]

    street_legend = ax.legend(handles=street_handles, loc="upper left", fontsize=13, framealpha=0.9, edgecolor="#DAD8D7")
    ax.add_artist(street_legend)
    ax.legend(handles=thread_handles, loc="upper right", fontsize=13, framealpha=0.9, edgecolor="#DAD8D7")

    ax.grid(which="major", axis="both", color="#DAD8D7", alpha=0.5, zorder=1)
    ax.spines[["top", "right"]].set_visible(False)
    ax.spines["left"].set_linewidth(1.1)
    ax.spines["bottom"].set_linewidth(1.1)

    ax.set_xlabel("Epoch", fontsize=12, labelpad=10)
    ax.xaxis.set_tick_params(pad=2, labelsize=11)
    ax.set_ylabel("Infoset Count", fontsize=12, labelpad=10)
    ax.yaxis.set_tick_params(pad=2, labelsize=11)

    ax.set_title("Infosets per Street vs Epoch", fontsize=16, weight="bold", alpha=0.8, loc="center", pad=15)


def main():
    if not os.path.exists(DATA_DIR):
        print(f"Error: {DATA_DIR}/ folder not found")
        return

    print("Loading infoset data...")
    infoset_data = load_infoset_data()
    if not infoset_data:
        print("No infoset data found!")
        return

    for name, info in infoset_data.items():
        print(f"  {name}: {len(info['entries'])} epochs")

    fig, ax = plt.subplots(1, 1, figsize=(16, 9), dpi=96)

    print("Plotting infoset counts...")
    plot_infosets(ax, infoset_data)

    plt.subplots_adjust(left=0.06, bottom=0.08, right=0.96, top=0.90)
    fig.patch.set_facecolor("white")

    os.makedirs("figs", exist_ok=True)
    plt.savefig("figs/infosets_graph.png", dpi=150, facecolor="white")
    print("Saved plot to figs/infosets_graph.png")


if __name__ == "__main__":
    main()
