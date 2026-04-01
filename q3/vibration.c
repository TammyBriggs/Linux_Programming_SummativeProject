#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <math.h>

/* =======================================================================
 * MEMORY HANDLING STRATEGY & TYPE CONVERSION:
 * We use PySequence_Fast to wrap the input object. If the input is already 
 * a list or tuple, this does NOT allocate new heap memory; it simply increments 
 * the reference count. We then use PySequence_Fast_ITEMS to get a raw C array 
 * of PyObject pointers (PyObject **). We iterate over this array and convert 
 * elements on-the-fly using PyFloat_AsDouble(). This guarantees O(1) auxiliary 
 * memory (no C-level array malloc required).
 * ======================================================================= */

// Helper function to validate input and extract the fast sequence
static PyObject* get_fast_sequence(PyObject *data) {
    if (!PyList_Check(data) && !PyTuple_Check(data)) {
        PyErr_SetString(PyExc_TypeError, "Input must be a list or tuple of floats.");
        return NULL;
    }
    PyObject *fast_seq = PySequence_Fast(data, "Expected a sequence");
    if (!fast_seq) return NULL;
    
    Py_ssize_t size = PySequence_Fast_GET_SIZE(fast_seq);
    if (size == 0) {
        PyErr_SetString(PyExc_ValueError, "Input data cannot be empty.");
        Py_DECREF(fast_seq);
        return NULL;
    }
    return fast_seq;
}

/* * 1. peak_to_peak(data)
 * Math: Max(X) - Min(X). 
 * Complexity: O(N) time, O(1) space. Single pass.
 */
static PyObject* peak_to_peak(PyObject *self, PyObject *args) {
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) return NULL;

    PyObject *fast_seq = get_fast_sequence(data);
    if (!fast_seq) return NULL;

    Py_ssize_t size = PySequence_Fast_GET_SIZE(fast_seq);
    PyObject **items = PySequence_Fast_ITEMS(fast_seq);

    double min_val = PyFloat_AsDouble(items[0]);
    double max_val = min_val;

    for (Py_ssize_t i = 1; i < size; i++) {
        double val = PyFloat_AsDouble(items[i]);
        if (PyErr_Occurred()) { Py_DECREF(fast_seq); return NULL; }
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
    }

    Py_DECREF(fast_seq);
    return PyFloat_FromDouble(max_val - min_val);
}

/* * 2. rms(data)
 * Math: sqrt( (x1^2 + x2^2 + ... + xn^2) / N )
 * Complexity: O(N) time, O(1) space. Single pass.
 */
static PyObject* rms(PyObject *self, PyObject *args) {
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) return NULL;

    PyObject *fast_seq = get_fast_sequence(data);
    if (!fast_seq) return NULL;

    Py_ssize_t size = PySequence_Fast_GET_SIZE(fast_seq);
    PyObject **items = PySequence_Fast_ITEMS(fast_seq);
    double sum_sq = 0.0;

    for (Py_ssize_t i = 0; i < size; i++) {
        double val = PyFloat_AsDouble(items[i]);
        if (PyErr_Occurred()) { Py_DECREF(fast_seq); return NULL; }
        sum_sq += val * val;
    }

    Py_DECREF(fast_seq);
    return PyFloat_FromDouble(sqrt(sum_sq / size));
}

/* * 3. std_dev(data)
 * Math: Sample Standard Deviation using Welford's Algorithm.
 * Numerical Stability: We use Welford's method (one-pass) instead of the naive 
 * two-pass (sum of squares) to prevent catastrophic cancellation of floating-point 
 * values when numbers are very close together.
 * Complexity: O(N) time, O(1) space.
 */
static PyObject* std_dev(PyObject *self, PyObject *args) {
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) return NULL;

    PyObject *fast_seq = get_fast_sequence(data);
    if (!fast_seq) return NULL;

    Py_ssize_t size = PySequence_Fast_GET_SIZE(fast_seq);
    if (size < 2) {
        PyErr_SetString(PyExc_ValueError, "Standard deviation requires at least 2 data points.");
        Py_DECREF(fast_seq);
        return NULL;
    }

    PyObject **items = PySequence_Fast_ITEMS(fast_seq);
    double mean = 0.0, M2 = 0.0;

    for (Py_ssize_t i = 0; i < size; i++) {
        double val = PyFloat_AsDouble(items[i]);
        if (PyErr_Occurred()) { Py_DECREF(fast_seq); return NULL; }
        
        // Welford's step
        double delta = val - mean;
        mean += delta / (i + 1);
        double delta2 = val - mean;
        M2 += delta * delta2;
    }

    Py_DECREF(fast_seq);
    double variance = M2 / (size - 1); // Sample variance (N-1)
    return PyFloat_FromDouble(sqrt(variance));
}

/* * 4. above_threshold(data, threshold)
 * Math: Count(x > threshold).
 * Complexity: O(N) time, O(1) space.
 */
static PyObject* above_threshold(PyObject *self, PyObject *args) {
    PyObject *data;
    double threshold;
    if (!PyArg_ParseTuple(args, "Od", &data, &threshold)) return NULL;

    PyObject *fast_seq = get_fast_sequence(data);
    if (!fast_seq) return NULL;

    Py_ssize_t size = PySequence_Fast_GET_SIZE(fast_seq);
    PyObject **items = PySequence_Fast_ITEMS(fast_seq);
    long count = 0;

    for (Py_ssize_t i = 0; i < size; i++) {
        double val = PyFloat_AsDouble(items[i]);
        if (PyErr_Occurred()) { Py_DECREF(fast_seq); return NULL; }
        if (val > threshold) count++;
    }

    Py_DECREF(fast_seq);
    return PyLong_FromLong(count);
}

/* * 5. summary(data)
 * Math: Computes Count, Mean, Min, Max in a single pass.
 * Complexity: O(N) time, O(1) space.
 */
static PyObject* summary(PyObject *self, PyObject *args) {
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) return NULL;

    PyObject *fast_seq = get_fast_sequence(data);
    if (!fast_seq) return NULL;

    Py_ssize_t size = PySequence_Fast_GET_SIZE(fast_seq);
    PyObject **items = PySequence_Fast_ITEMS(fast_seq);

    double sum = 0.0;
    double min_val = PyFloat_AsDouble(items[0]);
    double max_val = min_val;

    for (Py_ssize_t i = 0; i < size; i++) {
        double val = PyFloat_AsDouble(items[i]);
        if (PyErr_Occurred()) { Py_DECREF(fast_seq); return NULL; }
        
        sum += val;
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
    }
    
    Py_DECREF(fast_seq);

    // Build the Python Dictionary
    PyObject *dict = PyDict_New();
    PyDict_SetItemString(dict, "count", PyLong_FromSsize_t(size));
    PyDict_SetItemString(dict, "mean", PyFloat_FromDouble(sum / size));
    PyDict_SetItemString(dict, "min", PyFloat_FromDouble(min_val));
    PyDict_SetItemString(dict, "max", PyFloat_FromDouble(max_val));

    return dict;
}

// Method Table
static PyMethodDef VibrationMethods[] = {
    {"peak_to_peak", peak_to_peak, METH_VARARGS, "Calculate max - min."},
    {"rms", rms, METH_VARARGS, "Calculate root mean square."},
    {"std_dev", std_dev, METH_VARARGS, "Calculate sample standard deviation."},
    {"above_threshold", above_threshold, METH_VARARGS, "Count readings above a threshold."},
    {"summary", summary, METH_VARARGS, "Return dict with count, mean, min, max."},
    {NULL, NULL, 0, NULL}
};

// Module Definition
static struct PyModuleDef vibrationmodule = {
    PyModuleDef_HEAD_INIT,
    "vibration",
    "Industrial vibration analysis C extension.",
    -1,
    VibrationMethods
};

// Module Initialization
PyMODINIT_FUNC PyInit_vibration(void) {
    return PyModule_Create(&vibrationmodule);
}
