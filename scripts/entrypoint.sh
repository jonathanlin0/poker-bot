#!/bin/bash
python3 scripts/load_demo_data.py
exec ./poker_server "$@"
