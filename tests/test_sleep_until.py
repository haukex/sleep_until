import unittest
from time import time
from sleep_until import sleep_until

# NOTE I wrote a simple test that checks what happens when the system clock is adjusted:
# git checkout https://gist.github.com/haukex/f3955421834b3aacda963e73ab58f74e.git adj_sys_clk_test

class TestSleepUntil(unittest.TestCase):

    def test_sleep_util(self):
        dur_s = 10
        print(f"Running test for {dur_s} seconds...", flush=True, end=' ')
        got_times = []
        first_s = int(time())
        next_s = first_s
        for _ in range(dur_s):
            now_s = time()
            while next_s < now_s:
                next_s += 1
            sleep_until(next_s)
            got_times.append(time())
        self.assertEqual( len(got_times), dur_s )
        print('Deltas:', flush=True, end=' ')
        for i in range(dur_s):
            exp = first_s+i+1
            got = got_times[i]
            print(f"{got-exp:.6f}s", flush=True, end=' ')
            self.assertLessEqual( abs(got-exp), 0.050 )  # abs>50ms
            # the following two could possibly be relaxed?
            self.assertLessEqual( got-exp, 0.002 )  # >2ms or <0
            self.assertGreaterEqual( got-exp, 0 )
