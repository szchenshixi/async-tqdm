import os
import json
import tempfile
from typing import Dict

from tqdm import tqdm

FIFO = "/tmp/progress.pipe"

def update(jsonObj: dict):
    pid = jsonObj.get("pid", None)
    total = jsonObj.get("total", None)
    desc = jsonObj.get("desc", None)
    n = jsonObj.get("n", 1)

    b="{desc} {bar}{n_fmt}/{total_fmt} [{elapsed}<{remaining}, {rate_fmt}{postfix}]"
    if pBars.get(pid) is None:
        pBars[pid] = tqdm(total=total, desc=desc, bar_format=b)
        return
    elif desc != None:
        pBars[pid].set_description(desc=desc)
    pBars[pid].update(n=n)


try:
    os.mkfifo(FIFO)
except FileExistsError:
    pass
pBars: Dict[int, tqdm] = {}

print(f"Start serving progress bar at {FIFO}")
while (True):
    with open(FIFO, "r") as progress:
        for line in progress:
            jsonObj = None
            try:
                jsonObj = json.loads(line) # Load string
            except:
                print("Invalid json received for progress. Discard.")
                continue
            update(jsonObj)
