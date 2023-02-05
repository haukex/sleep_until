#!/usr/bin/env python3
import sys
from time import time
from sleep_until import sleep_until

dur_s = 10
if len(sys.argv)>1:
    dur_s = int(sys.argv[1])

print("Running test for {} seconds...".format(dur_s))
got_times = []
first_s = int(time())
next_s = first_s
for _ in range(dur_s):
    now_s = time()
    while next_s < now_s: next_s += 1
    sleep_until(next_s)
    got_times.append(time())

if len(got_times) != dur_s:
    raise ValueError("expected {0} times, got {1}".format(dur_s, len(got_times)))
for i in range(dur_s):
    exp = first_s+i+1
    got = got_times[i]
    if got-exp>0.001:  # 1ms
        raise ValueError("{0!r} {1!r}".format(exp, got))

print("PASS")
