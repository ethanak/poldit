
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poldit.h>

#define PyInt_Check PyLong_Check
#define PyInt_AsLong PyLong_AsLong
#define PyInt_FromLong PyLong_FromLong
#define PyString_Check PyUnicode_Check
#define PyString_AsString PyUnicode_AsUTF8
#define PyString_FromString PyUnicode_FromString
#define PyString_FromFormat PyUnicode_FromFormat


#define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
#define MOD_RETVAL NULL
#define MOD_DEF(ob, name, doc, methods) \
        static struct PyModuleDef moduledef = { \
            PyModuleDef_HEAD_INIT, name, doc, -1, methods, }; \
        ob = PyModule_Create(&moduledef);

#define STRING8 "y"

typedef struct {
    PyObject_HEAD;
    poldit bufr;
} poldit_Object;

static int PolObject_init(poldit_Object *self,
                        PyObject * args, PyObject *kwds)
{
    static char *kwlist[]={"col","base","sep","nat","dp","unit",NULL};
    poldit_Init(&self->bufr);
    int ic=0,ib=0,is=0,in=0;char *dp=NULL;char *un=NULL;
    PyArg_ParseTupleAndKeywords(args, kwds,"|iiiiss",kwlist,
        &ic, &ib, &is, &in, &dp,&un);
    //printf("%d\n", ic);
    if (ic) poldit_setColloquial(&self->bufr,1);
    if (ib) poldit_setDecibase(&self->bufr,1);
    if (is) poldit_setSepspell(&self->bufr,1);
    if (in) poldit_setNatural(&self->bufr,1);
    if (dp) poldit_setDecipoint(&self->bufr,dp);
    if (un) poldit_selUnits(&self->bufr,un);
        
    return 0;
}

static void PolObject_dealloc(poldit_Object *self)
{
    poldit_freeBuffer(&self->bufr);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *PO_Stringify(PyObject *self, PyObject *args,PyObject *kwargs)
{
    char *s;int ic=-1,ib=-1,is=-1,in=-1;char *dp=NULL;char *un=NULL;
    static char *kwlist[]={"text","col","base","sep","nat","dp","unit",NULL};
    uint32_t cacheflags;
    char cachedp[16]={0};
    
    if (!PyArg_ParseTupleAndKeywords(args,kwargs,"s|iiiiss",kwlist,&s,&ic,&ib,&is,&in,&dp,&un)) {
        return NULL;
    }
    poldit *buffer = &((poldit_Object *)self)->bufr;
    cacheflags = buffer->flags;
    if (ic>=0) poldit_setColloquial(buffer, ic != 0);
    if (ib>=0) poldit_setDecibase(buffer, ib != 0);
    if (is>=0) poldit_setSepspell(buffer, is != 0);
    if (in>=0) poldit_setNatural(buffer, in != 0);
    if (dp) {
        strcpy(cachedp, buffer->decipoint);
        poldit_setDecipoint(buffer,dp);
    }
    if (un) poldit_selUnits(buffer, un);
    poldit_resetBuffer(buffer);
    size_t len=poldit_Convert(buffer,s);
    poldit_allocBuffer(buffer, len);
    poldit_Convert(buffer, s);
    buffer->flags = cacheflags;
    if (dp) poldit_setDecipoint(buffer, cachedp);
    return Py_BuildValue("s",buffer->mem);
}

static PyObject *PO_Colloquial(PyObject *self, PyObject *args)
{
    int n=-1;
    if (!PyArg_ParseTuple(args,"|p",&n)) {
        return NULL;
    }
    if (n >= 0) poldit_setColloquial(&((poldit_Object *)self)->bufr,n);
    else n = poldit_getColloquial(&((poldit_Object *)self)->bufr);
    return Py_BuildValue("i",n);
}

static PyObject *PO_Decibase(PyObject *self, PyObject *args)
{
    int n=-1;
    if (!PyArg_ParseTuple(args,"|p",&n)) {
        return NULL;
    }
    if (n >= 0) poldit_setDecibase(&((poldit_Object *)self)->bufr,n);
    else n = poldit_getDecibase(&((poldit_Object *)self)->bufr);
    return Py_BuildValue("i",n);
}

static PyObject *PO_Sepspell(PyObject *self, PyObject *args)
{
    int n=-1;
    if (!PyArg_ParseTuple(args,"|p",&n)) {
        return NULL;
    }
    if (n >= 0) poldit_setSepspell(&((poldit_Object *)self)->bufr,n);
    else n = poldit_getSepspell(&((poldit_Object *)self)->bufr);
    return Py_BuildValue("i",n);
}

static PyObject *PO_Natural(PyObject *self, PyObject *args)
{
    int n=-1;
    if (!PyArg_ParseTuple(args,"|p",&n)) {
        return NULL;
    }
    if (n >= 0) poldit_setNatural(&((poldit_Object *)self)->bufr,n);
    else n = poldit_getNatural(&((poldit_Object *)self)->bufr);
    return Py_BuildValue("i",n);    poldit *buffer = &((poldit_Object *)self)->bufr;
}

static PyObject *PO_Decipoint(PyObject *self, PyObject *args)
{
    char * s = NULL;
    if (!PyArg_ParseTuple(args,"|s",&s)) {
        return NULL;
    }
    poldit *buffer = &((poldit_Object *)self)->bufr;
    if (s) poldit_setDecipoint(buffer,s);
    return Py_BuildValue("s",buffer->decipoint);
}

static PyObject *PO_selUnit(PyObject *self, PyObject *args)
{
    char * s = NULL;
    if (!PyArg_ParseTuple(args,"|s",&s)) {
        return NULL;
    }
    poldit *buffer = &((poldit_Object *)self)->bufr;
    if (s) poldit_selUnits(buffer,s);
    char buf[32];
    poldit_getSelUnits(buffer, buf);
    return Py_BuildValue("s",buf);
}

static PyObject *PO_uInClass(PyObject *self, PyObject *args)
{
    char * s;int i;
    if (!PyArg_ParseTuple(args,"is",&i,&s)) {
        return NULL;
    }
    if (i < 0 || !*s) Py_RETURN_NONE;
    char buf[80];
    i = poldit_unitType(i, *s, buf);
    if (i<0) Py_RETURN_NONE;
    return Py_BuildValue("(is)",i,buf);
}

    
static PyMethodDef polmod_methods[] = {
    {"unitInClass", (PyCFunction)PO_uInClass, METH_VARARGS,
     "unitInClass(idx, znak) zwraca (next_idx, opis) lub None\n"},
    {NULL,NULL, 0,NULL}
};

static PyMethodDef poldit_methods[] = {
    {"Stringify", (PyCFunction)PO_Stringify, METH_VARARGS | METH_KEYWORDS,
     "Stringify(txt=string) zwraca string\n\
  Opcjonalne argumenty (dotyczą tylko bieżącej konwersji):\n\
      col  = bool - ustaw tryb potoczny\n\
      base = bool - czytanie ułamków dziesiętnych\n\
      sep  = bool - wyraźna separacja przy literowaniu\n\
      nat  = bool - naturalizacja\n\
      unit = str  - selektor jednostek (patrz selUnit)\n\
      dp   = str  - reprezentacja fonetyczna kropki dziesiętnej\n"},
    {"Colloquial", (PyCFunction)PO_Colloquial, METH_VARARGS,
     "Colloquial(bool) ustawia tryb potoczny\n\
Colloquial() odczytuje ustawienie\n"},
    {"Decibase", (PyCFunction)PO_Decibase, METH_VARARGS,
     "Decibase(bool) ustawia tryb czytania\n\
podstawy ułamków dziesiętnych\n\
Decibase() - odczytuje ustawienie\n"},
    {"Sepspell", (PyCFunction)PO_Sepspell, METH_VARARGS,
     "Sepspell(bool) ustawia wyraźną separację grup\n\
przy literowaiu\n\
Sepspell() - odczytuje ustawienie"},
    {"Natural", (PyCFunction)PO_Natural, METH_VARARGS,
     "Natural(bool) ustawia naturalizację\n\
(usunięcie tzw. hiperpoprawności)\n\
Natural() - odczytuje ustawienie"},
    {"Decipoint", (PyCFunction)PO_Decipoint, METH_VARARGS,
     "Decipoint(str) ustawia reprezentację fonetyczną kropki dziesiętnej\n(domyślnie \"i\")\n\
Zwraca ustawioną reprezentację\n\
Decipoint() - odczytuje ustawienie\n"},
    {"selUnit", (PyCFunction)PO_selUnit, METH_VARARGS,
     "selUnit() - zwraca string z aktualnie rozpoznawanych typów\n\
selUnit(str) ustala rozpoznawane typy jednostek:\n"},
    {"unitInClass", (PyCFunction)PO_uInClass, METH_VARARGS,
     "unitInClass(idx, znak) zwraca (next_idx, opis) lub None\n"},
    {NULL,NULL, 0,NULL}
};

static PyTypeObject poldit_PolObjectType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "polditLL.Poldit",             /*tp_name*/
    sizeof(poldit_Object), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PolObject_dealloc,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0, /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Poldit(**kwargs)\n\
      col  = bool - ustaw tryb potoczny\n\
      base = bool - czytanie ułamków dziesiętnych\n\
      sep  = bool - wyraźna separacja przy literowaniu\n\
      nat  = bool - naturalizacja\n\
      unit = str  - selektor jednostek (patrz selUnit)\n\
      dp   = str  - reprezentacja fonetyczna kropki dziesiętnej\n",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    poldit_methods,         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PolObject_init,  /* tp_init */
};

static char uubuf[2048];
MOD_INIT(polditLL)

{
    PyObject* m;
    int i;
    for (i=0;poldit_methods[i].ml_name;i++) {
        if (strstr(poldit_methods[i].ml_name, "Unit")) {
            strcpy(uubuf,poldit_methods[i].ml_doc);
            strcat(uubuf,poldit_getUnitTypes());
            poldit_methods[i].ml_doc = uubuf;
            break;
        }
    }

    poldit_PolObjectType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&poldit_PolObjectType) < 0)
        return MOD_RETVAL;
    
    MOD_DEF(m, "polditLL",
                       "poldit",
                       polmod_methods);
    Py_INCREF(&poldit_PolObjectType);
    PyModule_AddObject(m, "Poldit", (PyObject *)&poldit_PolObjectType);
    return m;

}
