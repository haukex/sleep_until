sleep\_until
============

This module provides a function `sleep_until(seconds)`, which is like
`time.sleep()`, but it sleeps until the specified time of the system clock as
returned by `time.time()`. This can be used, for example, to schedule events at
a specific timestamp obtained from `datetime.datetime.timestamp()`.

It is implemented using `clock_nanosleep(2)` on POSIX systems and
`SetWaitableTimerEx` on Windows.

See the notes in `time.sleep()` on the behavior when interrupted and on
accuracy. Additionally, because this function uses the system clock as a
reference (`CLOCK_REALTIME` on Unix), this means the reference clock is
adjustable and may jump backwards.

Here is how one might implement a loop that executes at a fixed interval:

```python
from time import time
from sleep_until import sleep_until

interval_s = 1

# using "int" here to start on a full second
next_s = int(time()) + interval_s
while True:
    # calculate the next wakeup time and sleep until then
    now_s = time()
    # if the user's code takes longer than the interval, skip intervals
    while next_s < now_s: next_s += interval_s
    sleep_until(next_s)

    # run any user-specified code here
    print(time())
```
