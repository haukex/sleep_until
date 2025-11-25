sleep\_until
============

This module provides a function `sleep_until(deadline_seconds)`, which is like
`time.sleep()`, but it sleeps until the specified time of the system clock as
returned by `time.time()`. This can be used, for example, to schedule events at
a specific timestamp obtained from `datetime.datetime.timestamp()`.

See the notes in `time.sleep()` on the behavior when interrupted and on
accuracy. Additionally, because this function uses the system clock as a
reference (`CLOCK_REALTIME` on Unix), this means the reference clock is
adjustable and may jump backwards. Limited testing has shown that `sleep_until`
should still work in a predictable fashion (either returning immediately if
the system clock is adjusted back, or sleeping until the specified wall-clock
time if the system clock is adjusted forwards), *however*, this behavior can
currently *not* be guaranteed by this module, so be sure to test the behavior
on your system if you expect to be making adjustments to the system clock
while `sleep_until` is running (including via a background NTP service).

* On POSIX systems, `clock_nanosleep(2)` is used, so this must be available,
  along with the appropriate tools to compile the module.
* On Windows, `SetWaitableTimerEx` is used, in combination with
  `CREATE_WAITABLE_TIMER_HIGH_RESOLUTION` if available.
  Precompiled "wheels" for CPython are available on PyPI
  (wheels for other OSes are not provided because it is not
  guaranteed that `clock_nanosleep(2)` is available there).
* On Mac OS X, at the time of writing, `clock_nanosleep` is still not available,
  so the module currently does not build there.

Here is how one might implement a loop that executes at a specific interval:

```python
from time import time
from sleep_until import sleep_until

interval_s = 1

# using "int" here to start on a full second
next_s = int(time()) + interval_s
while True:
    # calculate the next wake-up time and sleep until then
    now_s = time()
    # if the user's code takes longer than the interval, skip intervals
    while next_s < now_s:
        next_s += interval_s
    sleep_until(next_s)

    # run any user-specified code here
    print(time())
```
