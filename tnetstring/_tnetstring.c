//
//  _tnetstring.c:  python module for fast encode/decode of typed-netstrings
//
//  You get the following functions:
//
//    dumps:  dump a python object to a tnetstring
//    loads:  parse tnetstring into a python object
//    pop:    parse tnetstring into a python object,
//            return it along with unparsed data.

#include <Python.h>

#include "tns_core.c"


static PyObject *_tnetstring_Error;
static PyObject *_tnetstring_LoadError;
static PyObject *_tnetstring_DumpError;


static PyObject*
_tnetstring_loads(PyObject* self, PyObject *args) 
{
  PyObject *string, *val;
  char *data;
  size_t len;

  if(!PyArg_UnpackTuple(args, "loads", 1, 1, &string)) {
      return NULL;
  }
  if(!PyString_Check(string)) {
      PyErr_SetString(PyExc_TypeError, "arg must be a string");
      return NULL;
  }
  Py_INCREF(string);

  data = PyString_AS_STRING(string);
  len = PyString_GET_SIZE(string);
  val = tns_parse(data, len, NULL);
  Py_DECREF(string);
  if(val == NULL) {
      return NULL;
  }

  return val;
}


static PyObject*
_tnetstring_pop(PyObject* self, PyObject *args) 
{
  PyObject *string, *val, *rest;
  char *data, *remain;
  size_t len;

  if(!PyArg_UnpackTuple(args, "pop", 1, 1, &string)) {
      return NULL;
  }
  if(!PyString_Check(string)) {
      PyErr_SetString(PyExc_TypeError, "arg must be a string");
      return NULL;
  }
  Py_INCREF(string);

  data = PyString_AS_STRING(string);
  len = PyString_GET_SIZE(string);
  val = tns_parse(data, len, &remain);
  Py_DECREF(string);
  if(val == NULL) {
      return NULL;
  }

  rest = PyString_FromStringAndSize(remain, len-(remain-data));
  return PyTuple_Pack(2, val, rest);
}


static PyObject*
_tnetstring_dumps(PyObject* self, PyObject *args, PyObject *kwds) 
{
  PyObject *object, *string;
  char *output, *odata, *sdata;
  size_t len=0;

  if(!PyArg_UnpackTuple(args, "dumps", 1, 1, &object)) {
      return NULL;
  }
  Py_INCREF(object);

  output = tns_render_reversed(object, &len);
  Py_DECREF(object);
  if(output == NULL) {
      return NULL;
  }

  string = PyString_FromStringAndSize(NULL,len);
  if(string == NULL) {
      return NULL;
  }
  sdata = PyString_AS_STRING(string);
  odata = output + len - 1;
  while(odata >= output) {
      *sdata = *odata;
      odata--;
      sdata++;
  }
  free(output);
  return string;
}


static PyMethodDef _tnetstring_methods[] = {
    {"loads",
     (PyCFunction)_tnetstring_loads,
     METH_VARARGS,
     PyDoc_STR("loads(string) -> object\n"
               "This function parses a tnetstring into a python object.")},

    {"pop",
     (PyCFunction)_tnetstring_pop,
     METH_VARARGS,
     PyDoc_STR("pop(string) -> (object, remain)\n"
               "This function parses a tnetstring into a python object.\n"
               "It returns a tuple giving the parsed object and a string\n"
               "containing any unparsed data.")},

    {"dumps",
     (PyCFunction)_tnetstring_dumps,
     METH_VARARGS,
     PyDoc_STR("dumps(object) -> string\n"
               "This function dumps a python object as a tnetstring.")},

    {NULL, NULL}
};


PyDoc_STRVAR(module_doc,
"Fast encoding/decoding of typed-netstrings."
);


PyMODINIT_FUNC
init_tnetstring(void)
{
  PyObject *m;

  m = Py_InitModule3("_tnetstring", _tnetstring_methods, module_doc);
  if(m == NULL) {
      return;
  }

  _tnetstring_Error = PyErr_NewException("_tnetstring.Error", NULL, NULL);
  if(_tnetstring_Error == NULL) {
      return;
  }
  Py_INCREF(_tnetstring_Error);
  PyModule_AddObject(m, "Error", _tnetstring_Error);

  _tnetstring_LoadError = PyErr_NewException("_tnetstring.LoadError",
                                              _tnetstring_Error,NULL);
  if(_tnetstring_LoadError == NULL) {
      return;
  }
  Py_INCREF(_tnetstring_LoadError);
  PyModule_AddObject(m, "LoadError", _tnetstring_LoadError);

  _tnetstring_DumpError = PyErr_NewException("_tnetstring.DumpError",
                                              _tnetstring_Error,NULL);
  if(_tnetstring_DumpError == NULL) {
      return;
  }
  Py_INCREF(_tnetstring_DumpError);
  PyModule_AddObject(m, "DumpError", _tnetstring_DumpError);
}


//  Functions to hook the parser core up to python.

static inline void
tns_parse_error(const char *errstr)
{
  PyErr_SetString(_tnetstring_LoadError, errstr);
}

static inline void
tns_render_error(const char *errstr)
{
  PyErr_SetString(_tnetstring_DumpError, errstr);
}

static inline void*
tns_parse_string(const char *data, size_t len)
{
  return PyString_FromStringAndSize(data, len);
}

static inline void*
tns_parse_integer(const char *data, size_t len)
{
  long long l;
  char *dataend;
  l = strtoll(data, &dataend, 10);
  if(dataend != data + len) {
      return NULL;
  }
  return PyLong_FromLongLong(l);
}

static inline void*
tns_parse_float(const char *data, size_t len)
{
  double d;
  char *dataend;
  d = strtod(data, &dataend);
  if(dataend != data + len) {
      return NULL;
  }
  return PyFloat_FromDouble(d);
}

static inline void*
tns_get_null(void)
{
  Py_INCREF(Py_None);
  return Py_None;
}

static inline void*
tns_get_true(void)
{
  Py_INCREF(Py_True);
  return Py_True;
}

static inline void*
tns_get_false(void)
{
  Py_INCREF(Py_False);
  return Py_False;
}

static inline void*
tns_new_dict(void)
{
  return PyDict_New();
}

static inline void*
tns_new_list(void)
{
  return PyList_New(0);
}

static inline void
tns_free_dict(void *dict)
{
  Py_DECREF(dict);
}

static inline void
tns_free_list(void *list)
{
  Py_DECREF(list);
}

static inline int
tns_add_to_dict(void *dict, void *key, void *item)
{
  int res;
  res = PyDict_SetItem(dict, key, item);
  Py_DECREF(key);
  Py_DECREF(item);
  if(res == -1) {
      return -1;
  }
  return 0;
}

static inline int
tns_add_to_list(void *list, void *item)
{
  int res;
  res = PyList_Append(list, item);
  Py_DECREF(item);
  if(res == -1) {
      return -1;
  }
  return 0;
}


static inline int
tns_render_string(void *val, tns_outbuf *outbuf)
{
    return tns_outbuf_rputs(outbuf, PyString_AS_STRING(val),
                                    PyString_GET_SIZE(val));
}

static inline int
tns_render_number(void *val, tns_outbuf *outbuf)
{
  PyObject *string;

  if(PyFloat_Check((PyObject*)val)) {
      string = PyObject_Repr(val);
  } else {
      string = PyObject_Str(val);
  }
  if(string == NULL) {
      return -1;
  }
  return tns_render_string(string, outbuf);
}

static inline int
tns_render_bool(void *val, tns_outbuf *outbuf)
{
  if(val == Py_True) {
      return tns_outbuf_rputs(outbuf, "true", 4);
  } else {
      return tns_outbuf_rputs(outbuf, "false", 5);
  }
}

static inline int
tns_render_dict(void *val, tns_outbuf *outbuf)
{
  PyObject *key, *item;
  Py_ssize_t pos = 0;

  while(PyDict_Next(val, &pos, &key, &item)) {
      if(tns_render_value(item, outbuf) == -1) {
          return -1;
      }
      if(tns_render_value(key, outbuf) == -1) {
          return -1;
      }
  }
  return 0;
}


static inline int
tns_render_list(void *val, tns_outbuf *outbuf)
{
  PyObject *item;
  Py_ssize_t idx;

  //  Remember, all output is in reverse.
  //  So we must write the last element first.
  idx = PyList_GET_SIZE(val) - 1;
  while(idx >= 0) {
      item = PyList_GET_ITEM(val, idx);
      if(tns_render_value(item, outbuf) == -1) {
          return -1;
      }
      idx--;
  }
  return 0;
}


static inline tns_type_tag
tns_get_type(void *val)
{
  if(val == Py_True || val == Py_False) {
    return tns_bool_tag;
  }
  if(val == Py_None) {
    return tns_null_tag;
  }
  if(PyInt_Check((PyObject*)val) || PyLong_Check((PyObject*)val)) {
    return tns_number_tag;
  }
  if(PyFloat_Check((PyObject*)val)) {
    return tns_number_tag;
  }
  if(PyString_Check((PyObject*)val)) {
    return tns_string_tag;
  }
  if(PyList_Check((PyObject*)val)) {
    return tns_list_tag;
  }
  if(PyDict_Check((PyObject*)val)) {
    return tns_dict_tag;
  }
  return 0;
}

