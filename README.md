# Poker Bot
By: Jonathan Lin

Accompanying writing for this repo: [jonathanlin.net/thoughts/poker-bot](https://jonathanlin.net/thoughts/poker-bot)

I highly recommend reading it if you plan to use any part of this repository.

## Requirements

Install cmake
```
sudo apt install -y cmake build-essential --no-update
```

## Precompute Equity
This step isn't necessary, but it's highly recommended. The precomputed equity speeds up convergence in early epochs. Running the program for a few hours should be enough.
```
# configure and build
cmake -B build && cmake --build build

# run
./build/precompute_equity
```

## Train
This process takes a few days on a 12 core (24 thread) CPU, where each core had a max boost clock of to 5.6 GHz. This may take more or less time depending on the specifications of your hardware.
```
# configure and build
cmake -B build && cmake --build build

# run
./build/poker_train -n "default-name" --num-threads 1
```

## Run Tests
```
# configure, build, and run
cmake -B build && cmake --build build && ./build/poker_tests

# sometimes there's WSL race conditions. So, you can build single threaded with the following
cmake -B build && cmake --build build -j1 && ./build/poker_tests
```

## Project Structure

- `include/config.hpp` - Config constants
- `include/` - Header files
- `src/` - Source files
- `frontend/` - Static files for demo website
- `data/` - Generated precomputed equity and training output
- `build/` - Cmake build output and compiled binaries

## Demo Website

Play against the trained bot through a web interface.

![demo website screenshot](figs/website-play.png)
### Requirements
SQLite3 development headers are required (pre-installed on macOS; on Linux: `sudo apt install libsqlite3-dev`).

### Build and Run
Run via docker
```
# build the image
docker build -t poker-bot-demo-server .

# run the container
docker stop poker-bot-demo-server && docker rm poker-bot-demo-server && docker run -d -p 9001:9001 --name poker-bot-demo-server -v ./data:/app/data poker-bot-demo-server
```

Run *without* Docker
```
# build
cmake -B build && cmake --build build --target poker_server

# run (requires trained weights at data/{experiment_name}/avg_strat.bin)
# an instance of the weights can be obtained by training an instance of the bot using the instructions above 
./build/poker_server
```

The server listens on port 9001. Can be locally accessed via `http://localhost:9001`.

## Visualizations

### Exploitability
Plots each experiment's exploitability per hand. Exploitability is the maximum expected profit an opponent can make against the bot if the exploiter always chooses the best response in each situation. The best response is the action that maximizes their EV. Exploitability will always be positive, but it should asymptotically approach 0 as the bot gets better.
```
python scripts/graph_exploitability.py
```

Example:
![exploitability graph](figs/exploitability_graph.png)