/*	$OpenBSD$	*/

/*
 * Copyright (c) 2014 Gilles Chehade <gilles@poolp.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <unistd.h>

#include <Python.h>

#include "smtpd-defines.h"
#include "smtpd-api.h"
#include "log.h"

static PyObject	*py_message_create;
static PyObject	*py_message_commit;
static PyObject	*py_message_delete;
static PyObject	*py_message_fd_r;
static PyObject	*py_message_corrupt;

static PyObject	*py_envelope_create;
static PyObject	*py_envelope_delete;
static PyObject	*py_envelope_update;
static PyObject	*py_envelope_load;
static PyObject	*py_envelope_walk;

static int
queue_python_message_create(uint32_t *msgid)
{
	PyObject *py_args;
	PyObject *py_ret;
	PyObject *py_msgid;
	PyObject *py_path;

	py_args  = PyTuple_New(0);
	py_ret = PyObject_CallObject(py_message_create, py_args);
	Py_DECREF(py_args);
	if (py_ret == NULL) {
		PyErr_Print();
		return (0);
	}
	*msgid = PyLong_AsUnsignedLong(py_ret);
	return (*msgid ? 1 : 0);
}

static int
queue_python_message_commit(uint32_t msgid, const char * path)
{
	PyObject *py_args;
	PyObject *py_ret;
	PyObject *py_msgid;
	PyObject *py_path;

	py_args  = PyTuple_New(2);
	py_msgid = PyLong_FromUnsignedLong(msgid);
	py_path  = PyString_FromString(path);

	PyTuple_SetItem(py_args, 0, py_msgid);
	PyTuple_SetItem(py_args, 1, py_path);

	py_ret = PyObject_CallObject(py_message_commit, py_args);
	Py_DECREF(py_args);
	if (py_ret == NULL) {
		PyErr_Print();
		return (0);
	}

	return (PyLong_AsUnsignedLong(py_ret) == 0 ? 0 : 1);
}

static int
queue_python_message_delete(uint32_t msgid)
{
	PyObject *py_args;
	PyObject *py_ret;
	PyObject *py_msgid;

	py_args  = PyTuple_New(1);
	py_msgid = PyLong_FromUnsignedLong(msgid);

	PyTuple_SetItem(py_args, 0, py_msgid);

	py_ret = PyObject_CallObject(py_message_delete, py_args);
	Py_DECREF(py_args);
	if (py_ret == NULL) {
		PyErr_Print();
		return (0);
	}
	return (1);
}

static int
queue_python_message_fd_r(uint32_t msgid)
{
	PyObject *py_args;
	PyObject *py_ret;
	PyObject *py_msgid;

	py_args  = PyTuple_New(1);
	py_msgid = PyLong_FromUnsignedLong(msgid);

	PyTuple_SetItem(py_args, 0, py_msgid);

	py_ret = PyObject_CallObject(py_message_fd_r, py_args);

	Py_DECREF(py_args);
	if (py_ret == NULL) {
		PyErr_Print();
		return (-1);
	}

	return PyLong_AsLong(py_ret);
}

static int
queue_python_message_corrupt(uint32_t msgid)
{
	PyObject *py_args;
	PyObject *py_ret;
	PyObject *py_msgid;

	py_args  = PyTuple_New(1);
	py_msgid = PyLong_FromUnsignedLong(msgid);

	PyTuple_SetItem(py_args, 0, py_msgid);

	py_ret = PyObject_CallObject(py_message_corrupt, py_args);

	Py_DECREF(py_args);
	if (py_ret == NULL) {
		PyErr_Print();
		return (0);
	}

	return (1);
}

static int
queue_python_envelope_create(uint32_t msgid, const char *buf, size_t len,
    uint64_t *evpid)
{
	PyObject *py_args;
	PyObject *py_ret;
	PyObject *py_msgid;
	PyObject *py_buffer;

	py_args   = PyTuple_New(2);
	py_msgid  = PyLong_FromUnsignedLong(msgid);
	py_buffer = PyBuffer_FromMemory(buf, len);

	PyTuple_SetItem(py_args, 0, py_msgid);
	PyTuple_SetItem(py_args, 1, py_buffer);

	py_ret = PyObject_CallObject(py_envelope_create, py_args);
	Py_DECREF(py_args);
	if (py_ret == NULL) {
		PyErr_Print();
		return (0);
	}

	*evpid = PyLong_AsUnsignedLongLong(py_ret);
	return (*evpid ? 1 : 0);
}

static int
queue_python_envelope_delete(uint64_t evpid)
{
	PyObject *py_args;
	PyObject *py_ret;
	PyObject *py_evpid;

	py_args  = PyTuple_New(1);
	py_evpid = PyLong_FromUnsignedLongLong(evpid);

	PyTuple_SetItem(py_args, 0, py_evpid);

	py_ret = PyObject_CallObject(py_envelope_delete, py_args);
	Py_DECREF(py_args);
	if (py_ret == NULL) {
		PyErr_Print();
		return (0);
	}
	return (1);
}

static int
queue_python_envelope_update(uint64_t evpid, const char *buf, size_t len)
{
	PyObject *py_args;
	PyObject *py_ret;
	PyObject *py_evpid;
	PyObject *py_buffer;

	py_args   = PyTuple_New(2);
	py_evpid  = PyLong_FromUnsignedLongLong(evpid);
	py_buffer = PyBuffer_FromMemory(buf, len);

	PyTuple_SetItem(py_args, 0, py_evpid);
	PyTuple_SetItem(py_args, 1, py_buffer);

	py_ret = PyObject_CallObject(py_envelope_update, py_args);
	Py_DECREF(py_args);
	if (py_ret == NULL) {
		PyErr_Print();
		return (0);
	}
	return (PyLong_AsUnsignedLongLong(py_ret) == 0 ? 0 : 1);
}

static int
queue_python_envelope_load(uint64_t evpid, char *buf, size_t len)
{
	PyObject *py_args;
	PyObject *py_ret;
	PyObject *py_evpid;
	PyObject *py_buffer;
	Py_buffer view;

	py_args   = PyTuple_New(1);
	py_evpid  = PyLong_FromUnsignedLongLong(evpid);

	PyTuple_SetItem(py_args, 0, py_evpid);

	py_ret = PyObject_CallObject(py_envelope_load, py_args);
	Py_DECREF(py_args);
	if (py_ret == NULL) {
		PyErr_Print();
		return (0);
	}

	if (PyObject_GetBuffer(py_ret, &view, PyBUF_SIMPLE) != 0) {
		PyErr_Print();
		return (0);
	}

	if (view.len >= len) {
		PyErr_Print();
		PyBuffer_Release(&view);
		return (0);
	}
	memset(buf, 0, len);
	memcpy(buf, view.buf, view.len);
	PyBuffer_Release(&view);

	log_warnx("evp: %s", buf);

	return (1);
}

static int
queue_python_envelope_walk(uint64_t *evpid, char *buf, size_t len)
{
	return (-1);
}

static int
queue_python_init(int server)
{
	queue_api_on_message_create(queue_python_message_create);
	queue_api_on_message_commit(queue_python_message_commit);
	queue_api_on_message_delete(queue_python_message_delete);
	queue_api_on_message_fd_r(queue_python_message_fd_r);
	queue_api_on_message_corrupt(queue_python_message_corrupt);
	queue_api_on_envelope_create(queue_python_envelope_create);
	queue_api_on_envelope_delete(queue_python_envelope_delete);
	queue_api_on_envelope_update(queue_python_envelope_update);
	queue_api_on_envelope_load(queue_python_envelope_load);
	queue_api_on_envelope_walk(queue_python_envelope_walk);

	return (1);
}

static PyMethodDef py_methods[] = {
	{ NULL, NULL, 0, NULL }
};

static char *
loadfile(const char * path)
{
	FILE	*f;
	off_t	 oz;
	size_t	 sz;
	char	*buf;

	if ((f = fopen(path, "r")) == NULL)
		err(1, "fopen");

	if (fseek(f, 0, SEEK_END) == -1)
		err(1, "fseek");

	oz = ftello(f);

	if (fseek(f, 0, SEEK_SET) == -1)
		err(1, "fseek");

	if (oz >= SIZE_MAX)
		errx(1, "too big");

	sz = oz;

	if ((buf = malloc(sz + 1)) == NULL)
		err(1, "malloc");

	if (fread(buf, 1, sz, f) != sz)
		err(1, "fread");

	buf[sz] = '\0';

	fclose(f);

	return (buf);
}

int
main(int argc, char **argv)
{
	int		ch;
	char	       *path;
	char	       *buf;
	PyObject       *self, *code, *module;

	log_init(1);

	while ((ch = getopt(argc, argv, "")) != -1) {
		switch (ch) {
		default:
			log_warnx("warn: backend-queue-python: bad option");
			return (1);
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		errx(1, "missing path");
	path = argv[0];


	Py_Initialize();
	self = Py_InitModule("queue", py_methods);

	buf = loadfile(path);
	code = Py_CompileString(buf, path, Py_file_input);
	free(buf);

	if (code == NULL) {
		PyErr_Print();
		log_warnx("warn: queue-python: failed to compile %s", path);
		return (1);
	}

	module = PyImport_ExecCodeModuleEx("queue_python", code, path);
	if (module == NULL) {
		PyErr_Print();
		log_warnx("warn: queue-python: failed to install module %s", path);
		return (1);
	}

	log_debug("debug: queue-python: starting...");

	if ((py_message_create = PyObject_GetAttrString(module, "message_create")) == NULL)
		goto nosuchmethod;
	if ((py_message_commit = PyObject_GetAttrString(module, "message_commit")) == NULL)
		goto nosuchmethod;
	if ((py_message_delete = PyObject_GetAttrString(module, "message_delete")) == NULL)
		goto nosuchmethod;
	if ((py_message_fd_r = PyObject_GetAttrString(module, "message_fd_r")) == NULL)
		goto nosuchmethod;
	if ((py_message_corrupt = PyObject_GetAttrString(module, "message_corrupt")) == NULL)
		goto nosuchmethod;
	if ((py_envelope_create = PyObject_GetAttrString(module, "envelope_create")) == NULL)
		goto nosuchmethod;
	if ((py_envelope_delete = PyObject_GetAttrString(module, "envelope_delete")) == NULL)
		goto nosuchmethod;
	if ((py_envelope_update = PyObject_GetAttrString(module, "envelope_update")) == NULL)
		goto nosuchmethod;
	if ((py_envelope_load = PyObject_GetAttrString(module, "envelope_load")) == NULL)
		goto nosuchmethod;
	if ((py_envelope_walk = PyObject_GetAttrString(module, "envelope_walk")) == NULL)
		goto nosuchmethod;

	queue_python_init(1);

	queue_api_no_chroot();
	queue_api_dispatch();

	return (0);

nosuchmethod:
	PyErr_Print();
	return (1);
}