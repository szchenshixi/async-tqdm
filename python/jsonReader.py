#!/bin/env python3
import os, json, time
from contextlib import suppress

FIFO = f"/tmp/{os.getlogin()}/progress.pipe"

with suppress(Exception):
    os.mkfifo(FIFO)

print(f"Reading from {FIFO}...")
while (True):
    with open(FIFO, "r") as fifo:
        for line in fifo:
            try:
                print("line: {}".format(line), end="")
                jsonObj = json.loads(line)
            except:
                raise
