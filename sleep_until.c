/** ***** ***** ***** Python Module sleep_until ***** ***** *****
 *
 * This file was derived from the Python source code as follows:
 * https://github.com/python/cpython/tree/3.14 [as of November 2025]
 * https://github.com/python/cpython/blob/9969c4280f/Python/pytime.c
 * https://github.com/python/cpython/blob/9969c4280f/Modules/timemodule.c
 * https://github.com/python/cpython/blob/9969c4280f/Include/internal/pycore_time.h
 */
#include "Python.h"
#include <time.h>

/* As of 3.13, several of the internal functions used here moved to the following internal header file,
 * while `PyTime_t` was made public. https://docs.python.org/3.13/whatsnew/3.13.html#c-api-changes */
#if PY_VERSION_HEX >= 0x030D0000
#define Py_BUILD_CORE
#include <internal/pycore_time.h>
#define _PyTime_t PyTime_t
#endif

#ifdef MS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* `_PyTime_As100Nanoseconds` wasn't added until Python 3.11. It is only called in one place from `timemodule.c`,
 * in the `MS_WINDOWS` part of `pysleep`, where it is called with `round=_PyTime_ROUND_CEILING`, and looking at
 * `_PyTime_As100Nanoseconds` in `pytime.c`, that's just a call to `pytime_divide` with `k=NS_TO_100NS=100`,
 * which is simply `if (t >= 0) return pytime_divide_round_up(t, k); else return t / k;`. And looking at
 * `pytime_divide_round_up` (also in `pytime.c`), that's just `PyTime_t q = t / k; if (t % k) q += 1; return q;`,
 * therefore we arrive at the following function. */
static _PyTime_t _pytime_to_100ns(const _PyTime_t t) {
    if (t >= 0) {
        _PyTime_t q = t / 100;
        if (t % 100)
            q += 1;
        return q;
    }
    else {
        return t / 100;
    }
}

/* From `timemodule.c` (used below) */
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif
static DWORD timer_flags = (DWORD)-1;

#endif /* MS_WINDOWS */

/* This code is derived from `pysleep` in `timemodule.c`. */
static int _sleepuntil(_PyTime_t deadline) {
    assert(deadline >= 0);
#ifndef MS_WINDOWS
    /* Note we assume that HAVE_CLOCK_NANOSLEEP is defined. */
    struct timespec timeout_abs;
    /* `pysleep` calculates `deadline` here by adding the `timeout` argument
     * to `PyTime_Monotonic`, but in `sleep_until`, the user is specifying
     * the `deadline` directly. The rest of this code is actually identical
     * to the non-`MS_WINDOWS` `pysleep` *except* that here, we specify
     * `CLOCK_REALTIME` instead of `CLOCK_MONOTONIC`. */
    if (_PyTime_AsTimespec(deadline, &timeout_abs) < 0)
        return -1;
    do {
        int err = 0;
        Py_BEGIN_ALLOW_THREADS
        err = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &timeout_abs, NULL);
        Py_END_ALLOW_THREADS
        if (err == 0)
            break;
        if (err != EINTR) {
            errno = err;
            PyErr_SetFromErrno(PyExc_OSError);
            return -1;
        }
        if (PyErr_CheckSignals()) /* sleep was interrupted by SIGINT */
            return -1;
    } while (1);
    return 0;

#else  // MS_WINDOWS
    /* Use the `_pytime_to_100ns` from above instead of `_PyTime_As100Nanoseconds`. */
    _PyTime_t timeout_100ns = _pytime_to_100ns(deadline);

    /* This bit is identical to the original `pysleep` (// comments are from the original) */
    // Maintain Windows Sleep() semantics for time.sleep(0)
    if (timeout_100ns == 0) {
        Py_BEGIN_ALLOW_THREADS
        // A value of zero causes the thread to relinquish the remainder of its
        // time slice to any other thread that is ready to run. If there are no
        // other threads ready to run, the function returns immediately, and
        // the thread continues execution.
        Sleep(0);
        Py_END_ALLOW_THREADS
        return 0;
    }

    /* This part is a bit interesting: I've renamed the original `relative_timeout` to `due_time`, and also, as per
     * https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setwaitabletimerex#parameters
     * it needs to be positive to specify an absolute time. Finally, we need to convert from
     * the Unix epoch (1970-01-01) to Windows epoch (1601-01-01); this is the inverse of what
     * is done in `py_get_system_clock` in `pytime.c`, which is where we get the value below from:
     * "11,644,473,600,000,000,000: number of nanoseconds between
     * the 1st january 1601 and the 1st january 1970 (369 years + 89 leap days)." */
    LARGE_INTEGER due_time;
    // No need to check for integer overflow, both types are signed
    assert(sizeof(due_time) == sizeof(timeout_100ns));
    // SetWaitableTimer(): Positive values indicate absolute time.
    due_time.QuadPart = timeout_100ns + 116444736000000000;

    /* The rest of this code is identical to the corresponding part of `pysleep`. */
    HANDLE timer = CreateWaitableTimerExW(NULL, NULL, timer_flags, TIMER_ALL_ACCESS);
    if (timer == NULL) {
        PyErr_SetFromWindowsErr(0);
        return -1;
    }

    if (!SetWaitableTimerEx(timer, &due_time,
                            0, // no period; the timer is signaled once
                            NULL, NULL, // no completion routine
                            NULL,  // no wake context; do not resume from suspend
                            0)) // no tolerable delay for timer coalescing
    {
        PyErr_SetFromWindowsErr(0);
        goto error;
    }

    // Only the main thread can be interrupted by SIGINT.
    // Signal handlers are only executed in the main thread.
    if (_PyOS_IsMainThread()) {
        HANDLE sigint_event = _PyOS_SigintEvent();

        while (1) {
            // Check for pending SIGINT signal before resetting the event
            if (PyErr_CheckSignals()) {
                goto error;
            }
            ResetEvent(sigint_event);

            HANDLE events[] = {timer, sigint_event};
            DWORD rc;

            Py_BEGIN_ALLOW_THREADS
            rc = WaitForMultipleObjects(Py_ARRAY_LENGTH(events), events,
                                        // bWaitAll
                                        FALSE,
                                        // No wait timeout
                                        INFINITE);
            Py_END_ALLOW_THREADS

            if (rc == WAIT_FAILED) {
                PyErr_SetFromWindowsErr(0);
                goto error;
            }

            if (rc == WAIT_OBJECT_0) {
                // Timer signaled: we are done
                break;
            }

            assert(rc == (WAIT_OBJECT_0 + 1));
            // The sleep was interrupted by SIGINT: restart sleeping
        }
    }
    else {
        DWORD rc;

        Py_BEGIN_ALLOW_THREADS
        rc = WaitForSingleObject(timer, INFINITE);
        Py_END_ALLOW_THREADS

        if (rc == WAIT_FAILED) {
            PyErr_SetFromWindowsErr(0);
            goto error;
        }

        assert(rc == WAIT_OBJECT_0);
        // Timer signaled: we are done
    }

    CloseHandle(timer);
    return 0;

error:
    CloseHandle(timer);
    return -1;
#endif
}

/* This function is mostly lifted from `time_sleep` in `timemodule.c`,
 * except I don't do the `PySys_Audit` call, which was added in 3.13. */
static PyObject * sleep_until(PyObject *self, PyObject *deadline_obj) {
    _PyTime_t deadline;
    if (_PyTime_FromSecondsObject(&deadline, deadline_obj, _PyTime_ROUND_TIMEOUT))
        return NULL;
    if (deadline < 0) {
        PyErr_SetString(PyExc_ValueError, "sleep_until deadline must be non-negative");
        return NULL;
    }
    if (_sleepuntil(deadline) != 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/* This bit is lifted from `time_exec` in `timemodule.c`. */
static int sleep_until_exec(PyObject *module) {
#if defined(MS_WINDOWS)
    if (timer_flags == (DWORD)-1) {
        DWORD test_flags = CREATE_WAITABLE_TIMER_HIGH_RESOLUTION;
        HANDLE timer = CreateWaitableTimerExW(NULL, NULL, test_flags,
                                                TIMER_ALL_ACCESS);
        if (timer == NULL) {
            // CREATE_WAITABLE_TIMER_HIGH_RESOLUTION is not supported.
            timer_flags = 0;
        }
        else {
            // CREATE_WAITABLE_TIMER_HIGH_RESOLUTION is supported.
            timer_flags = CREATE_WAITABLE_TIMER_HIGH_RESOLUTION;
            CloseHandle(timer);
        }
    }
#endif
    return 0;
}

/* The following are based on `time_methods`, `time_slots`, `timemodule`, and `PyInit_time` in `timemodule.c`. */

static PyMethodDef sleep_until_methods[] = {
    {"sleep_until",  sleep_until, METH_O,
        "sleep_until(deadline_seconds)\n\nDelay execution until the specified system clock time."},
    {NULL, NULL, 0, NULL}  /* sentinel */
};

static struct PyModuleDef_Slot sleep_until_slots[] = {
    {Py_mod_exec, sleep_until_exec},
#if PY_VERSION_HEX >= 0x030C0000
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#endif
#if PY_VERSION_HEX >= 0x030D0000
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
    {0, NULL}
};

static struct PyModuleDef sleep_until_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "sleep_until",
    .m_doc = "This module provides a function to sleep until a specific time.",
    .m_size = 0,
    .m_methods = sleep_until_methods,
    .m_slots = sleep_until_slots,
};

PyMODINIT_FUNC PyInit_sleep_until(void) {
    return PyModuleDef_Init(&sleep_until_module);
}
