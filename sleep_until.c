/* sleep_until module */
#include "Python.h"
#include <time.h>

#ifdef MS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* this code is heavily based on _PyTime_As100Nanoseconds from Python/pytime.c,
 * which was added in Python 3.11 */
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
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
  #define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif
static DWORD timer_flags = (DWORD)-1;
#endif /* MS_WINDOWS */

/* this code is heavily based on:
 * https://github.com/haukex/cpython/blob/10bf4d61af77/Modules/timemodule.c */

static int _sleepuntil(_PyTime_t deadline) {
    assert(deadline >= 0);
#ifndef MS_WINDOWS
    struct timespec timeout_abs;
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
    _PyTime_t timeout_100ns = _pytime_to_100ns(deadline);

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

    LARGE_INTEGER due_time;
    // No need to check for integer overflow, both types are signed
    assert(sizeof(due_time) == sizeof(timeout_100ns));
    // Adjust from Unix epoch (1970-01-01) to Windows epoch (1601-01-01)
    // (the inverse of what is done in py_get_system_clock)
    due_time.QuadPart = timeout_100ns + 116444736000000000;

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

static PyMethodDef sleep_until_methods[] = {
    {"sleep_until",  sleep_until, METH_O,
        "sleep_until(deadline_seconds)\n\nDelay execution until the specified system clock time."},
    {NULL, NULL, 0, NULL}  /* sentinel */
};

static struct PyModuleDef sleep_until_module = {
    PyModuleDef_HEAD_INIT,
    "sleep_until", /* name */
    "This module provides a function to sleep until a specific time.", /* documentation */
    -1, /* -1 = module keeps state in global variables */
    sleep_until_methods
};

PyMODINIT_FUNC PyInit_sleep_until(void) {
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
    return PyModule_Create(&sleep_until_module);
}
