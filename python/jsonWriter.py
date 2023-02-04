#!/bin/env python3
import os, json, time

TOTAL = 500
FIFO = f"/tmp/{os.getlogin()}/progress.pipe"
PID = os.getpid()

init = {
    "pid": PID,
    "total": TOTAL,
    "desc": "DummyTaskName",
    "status": "Running"
}

update = {
    "pid": PID,
    # "n": 1  # Optional
}

complete = {
    "pid": PID,
    "status": "Complete!"
}

print(f"Writing to {FIFO}...")
fifo = open(FIFO, "w")
json.dump(init, fifo)
fifo.write("\n")
for i in range(TOTAL):
    fifo = open(FIFO, "w")
    json.dump(update, fifo)
    fifo.write("\n")
    time.sleep(0.01)
json.dump(complete, fifo)
fifo.write("\n")
fifo.close()
