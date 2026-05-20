
/* Array object interface */
/* An array is a uniform list -- all items have the same type.
   The item type is restricted to simple C types like int or float */


#ifndef Py_ARRAYOBJECT_H
#define Py_ARRAYOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

struct PyArrayObject; /* Forward */

/* All possible arraydescr values are defined in the vector "descriptors"
 * below.  That's defined later because the appropriate get and set
 * functions aren't visible yet.
 */
typedef struct PyArrayDescr{
	int typecode;
    int itemsize;
    PyObject * (*getitem)(struct PyArrayObject *, Py_ssize_t);
    int (*setitem)(struct PyArrayObject *, Py_ssize_t, PyObject *);
} PyArrayDescr;

typedef struct PyArrayObject {
	PyObject_VAR_HEAD
    char *ob_item;
    Py_ssize_t allocated;
    struct PyArrayDescr *ob_descr;
    PyObject *weakreflist; /* List of weak references */
} PyArrayObject;

PyAPI_DATA(PyTypeObject) PyArray_Type;
PyAPI_DATA(struct PyArrayDescr) PyArrayDescriptors[];

#define PyArray_Check(op) ((op)->ob_type == &PyArray_Type)

PyAPI_FUNC(PyObject *) PyArray_New(Py_ssize_t size, struct PyArrayDescr *descr);

#ifdef __cplusplus
}
#endif
#endif /* !Py_ARRAYOBJECT_H */
