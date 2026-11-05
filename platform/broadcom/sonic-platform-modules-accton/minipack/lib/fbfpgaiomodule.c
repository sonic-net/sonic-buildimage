
#include <Python.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

//#define IDEBUG(...) printf(__VA_ARGS__)
#define IDEBUG(...)

#define FPGA_RESOURCE_NODE "/sys/devices/pci0000:00/0000:00:03.0/0000:05:00.0/resource0"
#define FPGA_RESOURCE_LENGTH 0x80000


static int hw_handle = -1;
static void *io_base = NULL;

static PyObject *fbfpgaio_hw_init(PyObject *self)
{
  const char fpga_resource_node[] = FPGA_RESOURCE_NODE;

  /* Open hardware resource node */
  hw_handle = open(fpga_resource_node, O_RDWR|O_SYNC);
  if (hw_handle == -1) {
    IDEBUG("[ERROR] %s: open hw resource node\n", __func__);
    return Py_False;
  }
  
  IDEBUG("[PASS] %s: open hw resource node\n", __func__);

  /* Mapping hardware resource */
  io_base = mmap(NULL, FPGA_RESOURCE_LENGTH, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_NORESERVE, hw_handle, 0);
  if (io_base == MAP_FAILED) {
    IDEBUG("[ERROR] %s: mapping resource node\n", __func__);
    perror("map_failed"); 
    fprintf(stderr,"%d %s\\n",errno,strerror(errno));
    return Py_False;
  }
  
  IDEBUG("[PASS] %s: mapping resource node\n", __func__);

  return Py_True; 
}

static PyObject *fbfpgaio_hw_release(PyObject *self)
{
  int retval = 0;

  if ((io_base != NULL) && (io_base != MAP_FAILED)) {
    retval = munmap(io_base, FPGA_RESOURCE_LENGTH);
    if (retval == 0) {
      IDEBUG("[PASS] %s: Unmapping hardware resources\n", __func__);
      close(hw_handle);
      return Py_True;
    }
  }

  IDEBUG("[ERROR] %s: unmapping resource node\n", __func__);
  return Py_False; 
}

static PyObject *fbfpgaio_hw_io(PyObject *self, PyObject *args)
{
  void *offset = NULL;
  /* We are not able to diffrentiate the input data between an unsigned value or a
     'None' object. We assume that the input data (if any) will be an unsigned integer.
     The default value of 'data' is larger than the max. number of unsigned integer.
     This value signify that the caller of this function does not input a data argument. */
  unsigned long input_data = 0x1FFFFFFFF;

  if (!PyArg_ParseTuple(args, "I|k", &offset, &input_data)) {
    return NULL;
  }

  if (input_data == 0x1FFFFFFFF) {
    // Read operation
    IDEBUG("Read operation\n");
    unsigned int *address = (unsigned int *) ((unsigned long) io_base + (unsigned long) offset);
    return Py_BuildValue("k", *address);
  } else {
    // Write operation
    IDEBUG("Write operation\n");
    unsigned int *address = (unsigned int *) ((unsigned long) io_base + (unsigned long) offset);
    unsigned int data = (unsigned int) (input_data & 0xFFFFFFFF);
    *address = data;

    Py_INCREF(Py_None);
    return Py_None;
  }
}

static PyMethodDef FbfpgaMethods[] = {
  { "hw_init", (PyCFunction) fbfpgaio_hw_init, METH_NOARGS, "Initialize resources for accessing FPGA" },
  { "hw_release", (PyCFunction) fbfpgaio_hw_release, METH_NOARGS, "Release resources for accessing FPGA" },
  { "hw_io", fbfpgaio_hw_io, METH_VARARGS, "Access FPGA" },
  { NULL, NULL, 0, NULL },
};

static char docstr[] = "\
1. hw_init():\n\
   return value: True/False\n\
2. hw_release():\n\
   return value: True/False\n\
3. hw_io(offset,[data])\n\
   return value:\n\
     In reading operation: data which is read from FPGA\n\
     In writing operation: None\n";

static struct PyModuleDef FbfpgaModule =
{
    PyModuleDef_HEAD_INIT,
    "fbfpgaio", /* name of module */
    docstr,
    -1,   /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    FbfpgaMethods
};

PyMODINIT_FUNC
PyInit_fbfpgaio(void)
{
  return PyModule_Create(&FbfpgaModule);
}


