#!/usr/bin/env python3
"""Test to confirm what happens when the system clock is adjusted during sleep_until.

The Windows documentation at
<https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setwaitabletimerex>
says regarding SetWaitableTimerEx:
"If the system time is adjusted, the due time of any outstanding absolute timers is adjusted."

The Ubuntu Linux documentation at
<https://manpages.ubuntu.com/manpages/kinetic/en/man2/clock_nanosleep.2.html#notes>
says regarding clock_nanosleep:
"POSIX.1 specifies that after changing the value of the CLOCK_REALTIME clock via
clock_settime(2), the new clock value shall be used to determine the time at which a
thread blocked on an absolute clock_nanosleep() will wake up; if the new clock value falls
past the end of the sleep interval, then the clock_nanosleep() call will return immediately."

Results (in the results_*.txt files) confirm this.
"""
import sys
from threading import Thread, Barrier
from datetime import datetime, timedelta, timezone
from time import sleep, monotonic
from sleep_until import sleep_until

def format_timedelta(td :timedelta) -> str:
    return '-' + str(-td) if td < timedelta(0) else str(td)

bar = Barrier(3)

class TimeSetThread(Thread):
    def __init__(self, delay_s, adjustment):
        super().__init__()
        self.delay_s = delay_s
        self.adj = adjustment
    def run(self):
        if sys.platform.startswith('win32'):
            # noinspection PyPackageRequirements,PyUnresolvedReferences
            import win32api
            def mysettime(dt :datetime):
                win32api.SetSystemTime( dt.year, dt.month, dt.isoweekday()%7, dt.day,
                                       dt.hour, dt.minute, dt.second, dt.microsecond//1000 )
        else:
            from time import clock_settime, CLOCK_REALTIME
            def mysettime(dt :datetime):
                clock_settime(CLOCK_REALTIME, dt.timestamp())
        bar.wait()
        monostart_s = monotonic()
        print(f"{monotonic()-monostart_s:6.3f}s Settime Thread started, sleeping {self.delay_s}s until adjustment")
        sleep(self.delay_s)
        now = datetime.now(timezone.utc)
        newtime = now + self.adj
        print(f"{monotonic()-monostart_s:6.3f}s Settime Thread adjusting time from {now.isoformat()} by {format_timedelta(self.adj)} to {newtime.isoformat()}")
        mysettime(newtime)
        print(f"{monotonic()-monostart_s:6.3f}s Settime Thread time adjusted, is now {datetime.now(timezone.utc).isoformat()}")

class SleeperThread(Thread):
    def __init__(self, relative):
        super().__init__()
        self.relative = relative
    def run(self):
        bar.wait()
        monostart_s = monotonic()
        now = datetime.now(timezone.utc)
        sleepto = now + self.relative
        print(f"{monotonic()-monostart_s:6.3f}s Sleep Thread started at {now.isoformat()}, now sleep_until for {format_timedelta(self.relative)} to {sleepto.isoformat()}")
        sleep_until( sleepto.timestamp() )
        print(f"{monotonic()-monostart_s:6.3f}s Sleep Thread woke at {datetime.now(timezone.utc).isoformat()}")

for adj in ( timedelta(seconds=-30), timedelta(seconds=30) ):
    print("Starting threads...")
    th1 = TimeSetThread( 30, adj )
    th2 = SleeperThread( timedelta(minutes=1) )
    th1.start()
    th2.start()
    bar.wait()
    th1.join()
    th2.join()
    print("Threads joined.")
