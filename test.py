#!/usr/bin/env python3
import sys
from time import clock_gettime, CLOCK_REALTIME
from sleep_until import sleep_until

dur_s = 10
if len(sys.argv)>1:
    dur_s = int(sys.argv[1])

print(f"Running test for {dur_s} seconds...")
got_times = []
first_s = int(clock_gettime(CLOCK_REALTIME))
next_s = first_s
for _ in range(dur_s):
    now_s = clock_gettime(CLOCK_REALTIME)
    while next_s < now_s: next_s += 1
    sleep_until(next_s)
    got_times.append(clock_gettime(CLOCK_REALTIME))

if len(got_times) != dur_s:
    raise ValueError(f"expected {dur_s} times, got {len(got_times)}")
for i in range(dur_s):
    exp = first_s+i+1
    got = got_times[i]
    if got-exp>0.001:  # 1ms
        raise ValueError(f"{exp=} {got=}")

print("PASS")
