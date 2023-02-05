#!/usr/bin/env python3
import sys
from time import time
from sleep_until import sleep_until

dur_s = 10
if len(sys.argv)>1:
    dur_s = int(sys.argv[1])

print("Running test for {} seconds...".format(dur_s), flush=True)
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
    if abs(got-exp)>0.050:
        raise ValueError("exp={0!r} got={1!r} abs>50ms".format(exp, got))
    elif got-exp>0.002 or got-exp<0:
        print("Warning: exp={0!r} got={1!r} >2ms or <0".format(exp, got), file=sys.stderr)

print("PASS")
