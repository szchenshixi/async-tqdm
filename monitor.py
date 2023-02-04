#!/bin/env python3
import os, json, tempfile
from typing import Dict
from contextlib import suppress


def isNotebook() -> bool:
    try:
        shell = get_ipython().__class__.__name__
        if shell == 'ZMQInteractiveShell':
            return True  # Jupyter notebook or qtconsole
        elif shell == 'TerminalInteractiveShell':
            return False  # Terminal running IPython
        else:
            return False  # Other type (?)
    except NameError:
        return False  # Probably standard Python interpreter


if isNotebook():
    from tqdm.notebook import tqdm
else:
    from tqdm import tqdm

FIFO = f"/tmp/{os.getlogin()}/progress.pipe"

# Since a console can run out of rows to display all progress bars, it is
# recommended to run this script on JupytorNote book if that happens.


def update(jsonObj: Dict[int, tqdm]):
    pid = jsonObj.get("pid", None)  # Identifier of the progress bar
    total = jsonObj.get("total", None)
    desc = jsonObj.get("desc", None)
    status = jsonObj.get("status", None)
    n = jsonObj.get("n", 1)

    b = "{desc} {bar}{n_fmt}/{total_fmt} [{elapsed}<{remaining}, {rate_fmt}{postfix}]"
    if pBars.get(pid) is None:
        pBars[pid] = tqdm(total=total,
                          desc=desc,
                          postfix=status,
                          bar_format=b,
                          dynamic_ncols=True)  # Allowing for window resizes
        return
    #  if desc != None:
    #      pBars[pid].set_description(desc=desc)
    elif status != None:
        pBars[pid].set_postfix_str(status)
        return
    else:
        pBars[pid].update(n=n)


os.makedirs(os.path.dirname(FIFO), exist_ok=True)
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
