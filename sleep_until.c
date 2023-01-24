/* sleep_until module */
#include "Python.h"
#include <time.h>

/* this code is heavily based on CPython's Modules/timemodule.c */

#ifdef MS_WINDOWS
#error "This module does not work on Windows."
#endif
#ifndef HAVE_CLOCK_NANOSLEEP
#error "This module requires clock_nanosleep."
#endif

static int _sleepuntil(_PyTime_t deadline) {
    assert(deadline >= 0);
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
}

static PyObject * sleep_until(PyObject *self, PyObject *deadline_obj) {
    _PyTime_t deadline;
    if (_PyTime_FromSecondsObject(&deadline, deadline_obj, _PyTime_ROUND_TIMEOUT))
        return NULL;
    if (deadline < 0) {
        PyErr_SetString(PyExc_ValueError, "deadline must be non-negative");
        return NULL;
    }
    if (_sleepuntil(deadline) != 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyMethodDef sleep_until_methods[] = {
    {"sleep_until",  sleep_until, METH_O,
        "sleep_until(deadline_seconds)\n\nDelay execution until the specified time of the CLOCK_REALTIME clock."},
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
    return PyModule_Create(&sleep_until_module);
}
