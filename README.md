sleep\_until
============

This module provides a function to sleep until a specific time
using `clock_nanosleep(2)`. It (currently) does not work on Windows.

The `sleep_until()` function takes a seconds value as an argument and
delays execution until the specified time of the `CLOCK_REALTIME` clock.

Here is how you might implement a loop that executes at a fixed interval:

```python
from time import clock_gettime, CLOCK_REALTIME
from sleep_until import sleep_until  # this module!

interval_s = 1

# using "int" here to start on a full second
next_s = int(clock_gettime(CLOCK_REALTIME)) + interval_s
while True:
    # calculate the next wakeup time and sleep until then
    now_s = clock_gettime(CLOCK_REALTIME)
    # if the user's code takes longer than the interval, skip intervals
    while next_s < now_s: next_s += interval_s
    sleep_until(next_s)

    # run any user-specified code here
    print(clock_gettime(CLOCK_REALTIME))
```
