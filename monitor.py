#!/bin/env python3
import os, json, tempfile
from typing import Dict
from tqdm import tqdm
from contextlib import suppress

FIFO = "/tmp/progress.pipe"


def update(jsonObj: Dict[int, tqdm]):
    pid = jsonObj.get("pid", None)
    total = jsonObj.get("total", None)
    desc = jsonObj.get("desc", None)
    n = jsonObj.get("n", 1)

    b = "{desc} {bar}{n_fmt}/{total_fmt} [{elapsed}<{remaining}, {rate_fmt}{postfix}]"
    if pBars.get(pid) is None:
        pBars[pid] = tqdm(total=total, desc=desc, bar_format=b)
        return
    elif desc != None:
        pBars[pid].set_description(desc=desc)
    pBars[pid].update(n=n)


with suppress(Exception):
    os.mkfifo(FIFO)

pBars: Dict[int, tqdm] = {}

print(f"Start serving progress bar at {FIFO}")
with suppress(Exception):
    while (True):
        with open(FIFO, "r") as progress:
            for line in progress:
                jsonObj = json.loads(line)  # Load string
                update(jsonObj)
