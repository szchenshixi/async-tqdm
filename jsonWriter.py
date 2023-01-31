import os
import json
import time

TOTAL = 100
FIFO = "/tmp/progress.pipe"
PID = os.getpid()

init = {
    "pid": PID,
    "total": TOTAL,
    "desc": "test"
}

update = {
    "pid": PID,
    # "n": 1  # Optional
}

complete = {
    "pid": PID,
    "desc": "complete"
}

fifo = open(FIFO, "w")
json.dump(init, fifo)
fifo.write("\n")
for i in range(TOTAL):
    fifo = open(FIFO, "w")
    json.dump(update, fifo)
    fifo.write("\n")
    time.sleep(0.1)
json.dump(complete, fifo)
fifo.write("\n")
fifo.close()
