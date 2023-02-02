import os
import json
import time

FIFO = '/tmp/progress.pipe'

#  os.mkfifo(FIFO)
while(True):
    with open(FIFO, "r") as fifo:
        for line in fifo:
            try:
                print("line: {}".format(line), end="")
                jsonObj = json.loads(line)
            except:
                raise
