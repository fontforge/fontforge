/* Copyright (C) 2025 FontForge Contributors
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 */

#include "ffprocess.h"

#include <ctype.h>
#include <errno.h>
#include <fontforge-config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#else
#include <strings.h> /* for strcasecmp on POSIX */
#endif

#ifndef _NO_PYTHON
#include <Python.h>
#endif

/* POSIX headers */
#if defined(__unix__) || defined(__APPLE__)
#define HAVE_POSIX 1
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "ffunistd.h"
extern char** environ;
#endif

/*
 * Check if we're running as a Python module (Python started us)
 * vs FontForge app (we started Python).
 */
#ifndef _NO_PYTHON
extern int FontForge_PythonInitialized(void); /* from python.c */
#endif

int ff_running_as_python_module(void) {
#ifndef _NO_PYTHON
    return !FontForge_PythonInitialized() && Py_IsInitialized();
#else
    return 0;
#endif
}

/* ==================== POSIX Implementation ==================== */

#ifdef HAVE_POSIX

static FFProcessResult posix_run_command(char** argv, const char* workdir,
                                         char** stdout_buf, char** stderr_buf) {
    int stdout_pipe[2] = {-1, -1};
    int stderr_pipe[2] = {-1, -1};
    pid_t pid;
    posix_spawn_file_actions_t actions;
    int status;
    FFProcessResult result = FF_PROCESS_FAILED;

    if (stdout_buf) *stdout_buf = NULL;
    if (stderr_buf) *stderr_buf = NULL;

    /* Create pipes for stdout/stderr capture */
    if (stdout_buf && pipe(stdout_pipe) != 0) return FF_PROCESS_FAILED;
    if (stderr_buf && pipe(stderr_pipe) != 0) {
        if (stdout_buf) {
            close(stdout_pipe[0]);
            close(stdout_pipe[1]);
        }
        return FF_PROCESS_FAILED;
    }

    /* Set up file actions for the child */
    if (posix_spawn_file_actions_init(&actions) != 0) {
        if (stdout_buf) {
            close(stdout_pipe[0]);
            close(stdout_pipe[1]);
        }
        if (stderr_buf) {
            close(stderr_pipe[0]);
            close(stderr_pipe[1]);
        }
        return FF_PROCESS_FAILED;
    }

    if (stdout_buf) {
        posix_spawn_file_actions_addclose(&actions, stdout_pipe[0]);
        posix_spawn_file_actions_adddup2(&actions, stdout_pipe[1],
                                         STDOUT_FILENO);
        posix_spawn_file_actions_addclose(&actions, stdout_pipe[1]);
    }
    if (stderr_buf) {
        posix_spawn_file_actions_addclose(&actions, stderr_pipe[0]);
        posix_spawn_file_actions_adddup2(&actions, stderr_pipe[1],
                                         STDERR_FILENO);
        posix_spawn_file_actions_addclose(&actions, stderr_pipe[1]);
    }

    /* TODO: handle workdir - posix_spawn doesn't support it directly,
     * would need posix_spawn_file_actions_addchdir_np on some systems
     * or fall back to fork/exec */
    (void)workdir;

    /* Spawn the process */
    int err = posix_spawnp(&pid, argv[0], &actions, NULL, argv, environ);
    posix_spawn_file_actions_destroy(&actions);

    if (err != 0) {
        if (stdout_buf) {
            close(stdout_pipe[0]);
            close(stdout_pipe[1]);
        }
        if (stderr_buf) {
            close(stderr_pipe[0]);
            close(stderr_pipe[1]);
        }
        return (err == ENOENT) ? FF_PROCESS_NOT_FOUND : FF_PROCESS_FAILED;
    }

    /* Close write ends in parent */
    if (stdout_buf) close(stdout_pipe[1]);
    if (stderr_buf) close(stderr_pipe[1]);

    /* Read stdout */
    if (stdout_buf) {
        size_t capacity = 4096, len = 0;
        char* buf = malloc(capacity);
        ssize_t n;
        while ((n = read(stdout_pipe[0], buf + len, capacity - len - 1)) > 0) {
            len += n;
            if (len + 1 >= capacity) {
                capacity *= 2;
                buf = realloc(buf, capacity);
            }
        }
        buf[len] = '\0';
        close(stdout_pipe[0]);
        *stdout_buf = buf;
    }

    /* Read stderr */
    if (stderr_buf) {
        size_t capacity = 4096, len = 0;
        char* buf = malloc(capacity);
        ssize_t n;
        while ((n = read(stderr_pipe[0], buf + len, capacity - len - 1)) > 0) {
            len += n;
            if (len + 1 >= capacity) {
                capacity *= 2;
                buf = realloc(buf, capacity);
            }
        }
        buf[len] = '\0';
        close(stderr_pipe[0]);
        *stderr_buf = buf;
    }

    /* Wait for child */
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        result = FF_PROCESS_OK;
    } else if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
        result = FF_PROCESS_NOT_FOUND;
    } else {
        result = FF_PROCESS_FAILED;
    }

    return result;
}

static FFProcessResult posix_run_command_binary(char** argv,
                                                const char* workdir,
                                                unsigned char** out_data,
                                                size_t* out_len) {
    int stdout_pipe[2] = {-1, -1};
    pid_t pid;
    posix_spawn_file_actions_t actions;
    int status;
    FFProcessResult result = FF_PROCESS_FAILED;

    *out_data = NULL;
    *out_len = 0;

    if (pipe(stdout_pipe) != 0) return FF_PROCESS_FAILED;

    if (posix_spawn_file_actions_init(&actions) != 0) {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return FF_PROCESS_FAILED;
    }

    posix_spawn_file_actions_addclose(&actions, stdout_pipe[0]);
    posix_spawn_file_actions_adddup2(&actions, stdout_pipe[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions, stdout_pipe[1]);

    (void)workdir;

    int err = posix_spawnp(&pid, argv[0], &actions, NULL, argv, environ);
    posix_spawn_file_actions_destroy(&actions);

    if (err != 0) {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return (err == ENOENT) ? FF_PROCESS_NOT_FOUND : FF_PROCESS_FAILED;
    }

    close(stdout_pipe[1]);

    /* Read binary data */
    size_t capacity = 4096, len = 0;
    unsigned char* buf = malloc(capacity);
    ssize_t n;
    while ((n = read(stdout_pipe[0], buf + len, capacity - len)) > 0) {
        len += n;
        if (len >= capacity) {
            capacity *= 2;
            buf = realloc(buf, capacity);
        }
    }
    close(stdout_pipe[0]);
    *out_data = buf;
    *out_len = len;

    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        result = FF_PROCESS_OK;
    } else if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
        result = FF_PROCESS_NOT_FOUND;
    } else {
        result = FF_PROCESS_FAILED;
    }

    return result;
}

#endif /* HAVE_POSIX */

/* ==================== Python Implementation ==================== */

#ifndef _NO_PYTHON

static FFProcessResult python_run_command(char** argv, const char* workdir,
                                          char** stdout_buf,
                                          char** stderr_buf) {
    PyObject *subprocess = NULL, *result = NULL, *args_list = NULL;
    PyObject *stdout_obj = NULL, *stderr_obj = NULL;
    FFProcessResult ret = FF_PROCESS_FAILED;

    if (stdout_buf) *stdout_buf = NULL;
    if (stderr_buf) *stderr_buf = NULL;

    subprocess = PyImport_ImportModule("subprocess");
    if (!subprocess) goto cleanup;

    /* Build args list */
    int argc = 0;
    while (argv[argc]) argc++;
    args_list = PyList_New(argc);
    for (int i = 0; i < argc; i++) {
        PyList_SetItem(args_list, i, PyUnicode_FromString(argv[i]));
    }

    /* Call subprocess.run(args, capture_output=True, cwd=workdir) */
    PyObject* kwargs = PyDict_New();
    PyDict_SetItemString(kwargs, "capture_output", Py_True);
    if (workdir) {
        PyDict_SetItemString(kwargs, "cwd", PyUnicode_FromString(workdir));
    }

    PyObject* run_func = PyObject_GetAttrString(subprocess, "run");
    PyObject* call_args = PyTuple_Pack(1, args_list);
    result = PyObject_Call(run_func, call_args, kwargs);
    Py_DECREF(run_func);
    Py_DECREF(call_args);
    Py_DECREF(kwargs);

    if (!result) {
        PyErr_Clear();
        ret = FF_PROCESS_NOT_FOUND;
        goto cleanup;
    }

    /* Check returncode */
    PyObject* returncode = PyObject_GetAttrString(result, "returncode");
    long rc = PyLong_AsLong(returncode);
    Py_DECREF(returncode);

    if (rc == 0) {
        ret = FF_PROCESS_OK;
    } else if (rc == 127) {
        ret = FF_PROCESS_NOT_FOUND;
    } else {
        ret = FF_PROCESS_FAILED;
    }

    /* Extract stdout */
    if (stdout_buf) {
        stdout_obj = PyObject_GetAttrString(result, "stdout");
        if (stdout_obj && PyBytes_Check(stdout_obj)) {
            Py_ssize_t len;
            char* data;
            PyBytes_AsStringAndSize(stdout_obj, &data, &len);
            *stdout_buf = malloc(len + 1);
            memcpy(*stdout_buf, data, len);
            (*stdout_buf)[len] = '\0';
        }
        Py_XDECREF(stdout_obj);
    }

    /* Extract stderr */
    if (stderr_buf) {
        stderr_obj = PyObject_GetAttrString(result, "stderr");
        if (stderr_obj && PyBytes_Check(stderr_obj)) {
            Py_ssize_t len;
            char* data;
            PyBytes_AsStringAndSize(stderr_obj, &data, &len);
            *stderr_buf = malloc(len + 1);
            memcpy(*stderr_buf, data, len);
            (*stderr_buf)[len] = '\0';
        }
        Py_XDECREF(stderr_obj);
    }

cleanup:
    Py_XDECREF(result);
    Py_XDECREF(args_list);
    Py_XDECREF(subprocess);
    return ret;
}

static FFProcessResult python_decompress_to_temp(const char* filename,
                                                 char** out_tmpfile) {
    PyObject *tempfile = NULL, *gzip = NULL, *bz2 = NULL, *lzma = NULL;
    PyObject *tmp = NULL, *compressed = NULL, *data = NULL;
    FFProcessResult ret = FF_PROCESS_FAILED;
    const char* ext;

    *out_tmpfile = NULL;

    ext = strrchr(filename, '.');
    if (!ext) return FF_PROCESS_FAILED;

    /* Import required modules */
    tempfile = PyImport_ImportModule("tempfile");
    if (!tempfile) goto cleanup;

    /* Determine compression type and decompress */
    if (strcmp(ext, ".gz") == 0) {
        gzip = PyImport_ImportModule("gzip");
        if (!gzip) goto cleanup;
        compressed = PyObject_CallMethod(gzip, "open", "ss", filename, "rb");
    } else if (strcmp(ext, ".bz2") == 0) {
        bz2 = PyImport_ImportModule("bz2");
        if (!bz2) goto cleanup;
        compressed = PyObject_CallMethod(bz2, "open", "ss", filename, "rb");
    } else if (strcmp(ext, ".lzma") == 0 || strcmp(ext, ".xz") == 0) {
        lzma = PyImport_ImportModule("lzma");
        if (!lzma) goto cleanup;
        compressed = PyObject_CallMethod(lzma, "open", "ss", filename, "rb");
    } else {
        goto cleanup;
    }

    if (!compressed) {
        PyErr_Clear();
        goto cleanup;
    }

    /* Read all data */
    data = PyObject_CallMethod(compressed, "read", NULL);
    PyObject_CallMethod(compressed, "close", NULL);
    if (!data || !PyBytes_Check(data)) {
        PyErr_Clear();
        goto cleanup;
    }

    /* Create temp file and write */
    /* Get the base filename without compression extension */
    const char* base = strrchr(filename, '/');
    base = base ? base + 1 : filename;
    size_t baselen = ext - base;
    char* suffix = malloc(baselen + 2);
    suffix[0] = '_';
    memcpy(suffix + 1, base, baselen);
    suffix[baselen + 1] = '\0';

    PyObject* kwargs = PyDict_New();
    PyDict_SetItemString(kwargs, "delete", Py_False);
    PyDict_SetItemString(kwargs, "suffix", PyUnicode_FromString(suffix));
    free(suffix);

    PyObject* ntf_class =
        PyObject_GetAttrString(tempfile, "NamedTemporaryFile");
    PyObject* call_args = PyTuple_New(0);
    tmp = PyObject_Call(ntf_class, call_args, kwargs);
    Py_DECREF(ntf_class);
    Py_DECREF(call_args);
    Py_DECREF(kwargs);

    if (!tmp) {
        PyErr_Clear();
        goto cleanup;
    }

    PyObject_CallMethod(tmp, "write", "O", data);
    PyObject* name_obj = PyObject_GetAttrString(tmp, "name");
    PyObject_CallMethod(tmp, "close", NULL);

    if (name_obj && PyUnicode_Check(name_obj)) {
        const char* name = PyUnicode_AsUTF8(name_obj);
        *out_tmpfile = strdup(name);
        ret = FF_PROCESS_OK;
    }
    Py_XDECREF(name_obj);

cleanup:
    Py_XDECREF(tmp);
    Py_XDECREF(data);
    Py_XDECREF(compressed);
    Py_XDECREF(gzip);
    Py_XDECREF(bz2);
    Py_XDECREF(lzma);
    Py_XDECREF(tempfile);
    return ret;
}

#endif /* _NO_PYTHON */

/* ==================== Public API ==================== */

FFProcessResult ff_run_command(char** argv, const char* workdir,
                               char** stdout_buf, char** stderr_buf) {
#ifndef _NO_PYTHON
    /* If we're a Python module, use Python directly */
    if (ff_running_as_python_module() && Py_IsInitialized()) {
        return python_run_command(argv, workdir, stdout_buf, stderr_buf);
    }
#endif

#ifdef HAVE_POSIX
    FFProcessResult r =
        posix_run_command(argv, workdir, stdout_buf, stderr_buf);
    if (r == FF_PROCESS_OK || r == FF_PROCESS_NOT_FOUND) {
        return r;
    }
    /* POSIX failed, try Python fallback */
#endif

#ifndef _NO_PYTHON
    if (Py_IsInitialized()) {
        return python_run_command(argv, workdir, stdout_buf, stderr_buf);
    }
#endif

    return FF_PROCESS_NO_BACKEND;
}

FFProcessResult ff_run_command_binary(char** argv, const char* workdir,
                                      unsigned char** out_data,
                                      size_t* out_len) {
    /* For binary data, POSIX is preferred even as Python module
     * because Python's subprocess handles binary better with direct pipes */
#ifdef HAVE_POSIX
    FFProcessResult r =
        posix_run_command_binary(argv, workdir, out_data, out_len);
    if (r == FF_PROCESS_OK) {
        return r;
    }
#endif

    /* TODO: Python fallback for binary data */
    return FF_PROCESS_NO_BACKEND;
}

FFProcessResult ff_decompress_to_temp(const char* filename,
                                      char** out_tmpfile) {
#ifndef _NO_PYTHON
    /* If we're a Python module, use Python's gzip/bz2/lzma directly */
    if (ff_running_as_python_module() && Py_IsInitialized()) {
        return python_decompress_to_temp(filename, out_tmpfile);
    }
#endif

#ifdef HAVE_POSIX
    /* Try external decompressor */
    const char* ext = strrchr(filename, '.');
    if (!ext) return FF_PROCESS_FAILED;

    const char* cmd = NULL;
    if (strcmp(ext, ".gz") == 0)
        cmd = "gunzip";
    else if (strcmp(ext, ".bz2") == 0)
        cmd = "bunzip2";
    else if (strcmp(ext, ".lzma") == 0 || strcmp(ext, ".xz") == 0)
        cmd = "unlzma";

    if (cmd) {
        char* argv[] = {(char*)cmd, "-c", (char*)filename, NULL};
        unsigned char* data = NULL;
        size_t len = 0;

        FFProcessResult r = ff_run_command_binary(argv, NULL, &data, &len);
        if (r == FF_PROCESS_OK && data && len > 0) {
            /* Write to temp file */
            char tmppath[] = "/tmp/ffdecomp_XXXXXX";
            int fd = mkstemp(tmppath);
            if (fd >= 0) {
                write(fd, data, len);
                close(fd);
                *out_tmpfile = strdup(tmppath);
                free(data);
                return FF_PROCESS_OK;
            }
            free(data);
        }
        if (data) free(data);
        /* Fall through to Python fallback */
    }
#endif

#ifndef _NO_PYTHON
    if (Py_IsInitialized()) {
        return python_decompress_to_temp(filename, out_tmpfile);
    }
#endif

    return FF_PROCESS_NO_BACKEND;
}

FFProcessResult ff_extract_from_archive(const char* archive, const char* member,
                                        char** out_tmpdir, char** out_file) {
    /* TODO: Implement archive extraction
     * This is more complex - needs to handle tar, zip, etc.
     * For now, return NO_BACKEND and let caller handle it
     */
    (void)archive;
    (void)member;
    (void)out_tmpdir;
    (void)out_file;
    return FF_PROCESS_NO_BACKEND;
}

/* ==================== Compression API ==================== */

/* Extension table - must match FFCompressionType enum order */
static const char* compression_extensions[] = {
    "",      /* FF_COMPRESS_NONE */
    ".gz",   /* FF_COMPRESS_GZ */
    ".bz2",  /* FF_COMPRESS_BZ2 */
    ".lzma", /* FF_COMPRESS_LZMA */
    ".xz",   /* FF_COMPRESS_XZ */
    ".Z"     /* FF_COMPRESS_Z */
};

/* Command table for POSIX compression/decompression */
static const struct {
    const char* compress_cmd;
    const char* decompress_cmd;
} compression_commands[] = {
    {NULL, NULL},          /* FF_COMPRESS_NONE */
    {"gzip", "gunzip"},    /* FF_COMPRESS_GZ */
    {"bzip2", "bunzip2"},  /* FF_COMPRESS_BZ2 */
    {"lzma", "unlzma"},    /* FF_COMPRESS_LZMA */
    {"xz", "unxz"},        /* FF_COMPRESS_XZ */
    {"compress", "gunzip"} /* FF_COMPRESS_Z */
};

FFCompressionType ff_compression_type(const char* filename) {
    const char* ext;

    if (!filename) return FF_COMPRESS_NONE;

    ext = strrchr(filename, '.');
    if (!ext) return FF_COMPRESS_NONE;

    if (strcmp(ext, ".gz") == 0) return FF_COMPRESS_GZ;
    if (strcmp(ext, ".bz2") == 0 || strcmp(ext, ".bz") == 0)
        return FF_COMPRESS_BZ2;
    if (strcmp(ext, ".lzma") == 0) return FF_COMPRESS_LZMA;
    if (strcmp(ext, ".xz") == 0) return FF_COMPRESS_XZ;
    if (strcmp(ext, ".Z") == 0) return FF_COMPRESS_Z;

    return FF_COMPRESS_NONE;
}

FFCompressionType ff_compression_from_legacy(int sf_compression) {
    /* Legacy SplineFont.compression values:
     * 0 = none, 1 = gz, 2 = bz2, 3 = bz (same as bz2), 4 = Z, 5 = lzma
     */
    switch (sf_compression) {
        case 0:
            return FF_COMPRESS_NONE;
        case 1:
            return FF_COMPRESS_GZ;
        case 2:
        case 3:
            return FF_COMPRESS_BZ2; /* Both .bz2 and .bz map to BZ2 */
        case 4:
            return FF_COMPRESS_Z;
        case 5:
            return FF_COMPRESS_LZMA;
        default:
            return FF_COMPRESS_NONE;
    }
}

int ff_compression_to_legacy(FFCompressionType type) {
    /* Convert to legacy SplineFont.compression values:
     * 0 = none, 1 = gz, 2 = bz2, 4 = Z, 5 = lzma
     * Note: XZ is not in the legacy format, map to lzma (5)
     */
    switch (type) {
        case FF_COMPRESS_NONE:
            return 0;
        case FF_COMPRESS_GZ:
            return 1;
        case FF_COMPRESS_BZ2:
            return 2;
        case FF_COMPRESS_Z:
            return 4;
        case FF_COMPRESS_LZMA:
            return 5;
        case FF_COMPRESS_XZ:
            return 5; /* Map XZ to lzma for legacy */
        default:
            return 0;
    }
}

const char* ff_compression_ext(FFCompressionType type) {
    if (type < 0 || type >= FF_COMPRESS_COUNT) {
        return "";
    }
    return compression_extensions[type];
}

/* Python compression helper */
#ifndef _NO_PYTHON
static FFProcessResult python_compress_file(const char* filename,
                                            FFCompressionType type) {
    PyObject *module = NULL, *infile = NULL, *outfile = NULL, *data = NULL;
    FFProcessResult ret = FF_PROCESS_FAILED;
    char* outpath = NULL;
    size_t len;

    if (!Py_IsInitialized()) return FF_PROCESS_NO_BACKEND;

    /* Build output filename */
    len = strlen(filename) + strlen(ff_compression_ext(type)) + 1;
    outpath = malloc(len);
    sprintf(outpath, "%s%s", filename, ff_compression_ext(type));

    /* Read input file */
    infile = PyObject_CallMethod(PyImport_ImportModule("builtins"), "open",
                                 "ss", filename, "rb");
    if (!infile) {
        PyErr_Clear();
        goto cleanup;
    }
    data = PyObject_CallMethod(infile, "read", NULL);
    PyObject_CallMethod(infile, "close", NULL);
    Py_DECREF(infile);
    infile = NULL;
    if (!data) {
        PyErr_Clear();
        goto cleanup;
    }

    /* Import compression module and write */
    switch (type) {
        case FF_COMPRESS_GZ:
            module = PyImport_ImportModule("gzip");
            break;
        case FF_COMPRESS_BZ2:
            module = PyImport_ImportModule("bz2");
            break;
        case FF_COMPRESS_LZMA:
        case FF_COMPRESS_XZ:
            module = PyImport_ImportModule("lzma");
            break;
        default:
            goto cleanup;
    }
    if (!module) {
        PyErr_Clear();
        goto cleanup;
    }

    outfile = PyObject_CallMethod(module, "open", "ss", outpath, "wb");
    if (!outfile) {
        PyErr_Clear();
        goto cleanup;
    }

    PyObject_CallMethod(outfile, "write", "O", data);
    PyObject_CallMethod(outfile, "close", NULL);

    /* Remove original file */
    remove(filename);
    ret = FF_PROCESS_OK;

cleanup:
    Py_XDECREF(outfile);
    Py_XDECREF(data);
    Py_XDECREF(infile);
    Py_XDECREF(module);
    free(outpath);
    return ret;
}

static FFProcessResult python_decompress_in_place(const char* filename) {
    PyObject *module = NULL, *infile = NULL, *outfile = NULL, *data = NULL;
    FFProcessResult ret = FF_PROCESS_FAILED;
    FFCompressionType type;
    char* outpath = NULL;
    const char* ext;
    size_t baselen;

    if (!Py_IsInitialized()) return FF_PROCESS_NO_BACKEND;

    type = ff_compression_type(filename);
    if (type == FF_COMPRESS_NONE) return FF_PROCESS_FAILED;

    /* Build output filename (remove extension) */
    ext = strrchr(filename, '.');
    baselen = ext - filename;
    outpath = malloc(baselen + 1);
    memcpy(outpath, filename, baselen);
    outpath[baselen] = '\0';

    /* Import compression module and read */
    switch (type) {
        case FF_COMPRESS_GZ:
            module = PyImport_ImportModule("gzip");
            break;
        case FF_COMPRESS_BZ2:
            module = PyImport_ImportModule("bz2");
            break;
        case FF_COMPRESS_LZMA:
        case FF_COMPRESS_XZ:
            module = PyImport_ImportModule("lzma");
            break;
        default:
            goto cleanup;
    }
    if (!module) {
        PyErr_Clear();
        goto cleanup;
    }

    infile = PyObject_CallMethod(module, "open", "ss", filename, "rb");
    if (!infile) {
        PyErr_Clear();
        goto cleanup;
    }
    data = PyObject_CallMethod(infile, "read", NULL);
    PyObject_CallMethod(infile, "close", NULL);
    Py_DECREF(infile);
    infile = NULL;
    if (!data) {
        PyErr_Clear();
        goto cleanup;
    }

    /* Write decompressed data */
    outfile = PyObject_CallMethod(PyImport_ImportModule("builtins"), "open",
                                  "ss", outpath, "wb");
    if (!outfile) {
        PyErr_Clear();
        goto cleanup;
    }
    PyObject_CallMethod(outfile, "write", "O", data);
    PyObject_CallMethod(outfile, "close", NULL);

    /* Remove compressed file */
    remove(filename);
    ret = FF_PROCESS_OK;

cleanup:
    Py_XDECREF(outfile);
    Py_XDECREF(data);
    Py_XDECREF(infile);
    Py_XDECREF(module);
    free(outpath);
    return ret;
}
#endif /* _NO_PYTHON */

FFProcessResult ff_compress_file(const char* filename, FFCompressionType type) {
    if (type == FF_COMPRESS_NONE || type >= FF_COMPRESS_COUNT) {
        return FF_PROCESS_FAILED;
    }

#ifndef _NO_PYTHON
    /* If we're a Python module, use Python directly */
    if (ff_running_as_python_module() && Py_IsInitialized()) {
        return python_compress_file(filename, type);
    }
#endif

#ifdef HAVE_POSIX
    {
        const char* cmd = compression_commands[type].compress_cmd;
        if (cmd) {
            char* argv[] = {(char*)cmd, (char*)filename, NULL};
            FFProcessResult r = ff_run_command(argv, NULL, NULL, NULL);
            if (r == FF_PROCESS_OK) {
                return r;
            }
            /* Fall through to Python fallback */
        }
    }
#endif

#ifndef _NO_PYTHON
    if (Py_IsInitialized()) {
        return python_compress_file(filename, type);
    }
#endif

    return FF_PROCESS_NO_BACKEND;
}

FFProcessResult ff_decompress_in_place(const char* filename) {
    FFCompressionType type = ff_compression_type(filename);

    if (type == FF_COMPRESS_NONE) {
        return FF_PROCESS_FAILED;
    }

#ifndef _NO_PYTHON
    /* If we're a Python module, use Python directly */
    if (ff_running_as_python_module() && Py_IsInitialized()) {
        return python_decompress_in_place(filename);
    }
#endif

#ifdef HAVE_POSIX
    {
        const char* cmd = compression_commands[type].decompress_cmd;
        if (cmd) {
            char* argv[] = {(char*)cmd, (char*)filename, NULL};
            FFProcessResult r = ff_run_command(argv, NULL, NULL, NULL);
            if (r == FF_PROCESS_OK) {
                return r;
            }
            /* Fall through to Python fallback */
        }
    }
#endif

#ifndef _NO_PYTHON
    if (Py_IsInitialized()) {
        return python_decompress_in_place(filename);
    }
#endif

    return FF_PROCESS_NO_BACKEND;
}

/* ==================== MIME Type ==================== */

/* Extension to MIME type table - sorted by extension for bsearch */
static const char* ext_mime_table[][2] = {
    {"bdf", "application/x-font-bdf"},
    {"bin", "application/x-macbinary"},
    {"bmp", "image/bmp"},
    {"bz2", "application/x-compressed"},
    {"c", "text/c"},
    {"cff", "application/x-font-type1"},
    {"cid", "application/x-font-cid"},
    {"css", "text/css"},
    {"dfont", "application/x-mac-dfont"},
    {"eps", "text/ps"},
    {"gai", "font/otf"},
    {"gif", "image/gif"},
    {"gz", "application/x-compressed"},
    {"h", "text/h"},
    {"hqx", "application/x-mac-binhex40"},
    {"html", "text/html"},
    {"jpeg", "image/jpeg"},
    {"jpg", "image/jpeg"},
    {"mov", "video/quicktime"},
    {"o", "application/x-object"},
    {"obj", "application/x-object"},
    {"otb", "font/otf"},
    {"otf", "font/otf"},
    {"pcf", "application/x-font-pcf"},
    {"pdf", "application/pdf"},
    {"pfa", "application/x-font-type1"},
    {"pfb", "application/x-font-type1"},
    {"png", "image/png"},
    {"ps", "text/ps"},
    {"pt3", "application/x-font-type1"},
    {"ras", "image/x-cmu-raster"},
    {"rgb", "image/x-rgb"},
    {"rpm", "application/x-compressed"},
    {"sfd", "application/vnd.font-fontforge-sfd"},
    {"sgi", "image/x-sgi"},
    {"snf", "application/x-font-snf"},
    {"svg", "image/svg+xml"},
    {"tar", "application/x-tar"},
    {"tbz", "application/x-compressed"},
    {"text", "text/plain"},
    {"tgz", "application/x-compressed"},
    {"tif", "image/tiff"},
    {"tiff", "image/tiff"},
    {"ttf", "font/ttf"},
    {"txt", "text/plain"},
    {"wav", "audio/wave"},
    {"woff", "font/woff"},
    {"woff2", "font/woff2"},
    {"xbm", "image/x-xbitmap"},
    {"xml", "text/xml"},
    {"xpm", "image/x-xpixmap"},
    {"z", "application/x-compressed"},
    {"zip", "application/x-compressed"},
};

static int mime_ext_compare(const void* key, const void* elem) {
    const char* ext = (const char*)key;
    const char* const* entry = (const char* const*)elem;
    return strcasecmp(ext, entry[0]);
}

static char* mime_from_extension(const char* path) {
    const char *filename, *dot, *ext;
    size_t ext_len;
    char* ext_lower;

    /* Get filename part */
    filename = strrchr(path, '/');
#ifdef _WIN32
    const char* bs = strrchr(path, '\\');
    if (bs && (!filename || bs > filename)) filename = bs;
#endif
    filename = filename ? filename + 1 : path;

    /* Get extension */
    dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        /* No extension - check special filenames */
        if (strcasecmp(filename, "makefile") == 0 ||
            strcasecmp(filename, "makefile~") == 0) {
            return strdup("application/x-makefile");
        }
        if (strcasecmp(filename, "core") == 0) {
            return strdup("application/x-core");
        }
        return NULL;
    }

    ext = dot + 1;
    ext_len = strlen(ext);

    /* Strip trailing ~ from backup files */
    if (ext_len > 0 && ext[ext_len - 1] == '~') {
        ext_len--;
    }
    if (ext_len == 0) return NULL;

    /* Make lowercase copy for comparison */
    ext_lower = (char*)malloc(ext_len + 1);
    if (!ext_lower) return NULL;
    for (size_t i = 0; i < ext_len; i++) {
        ext_lower[i] = (char)tolower((unsigned char)ext[i]);
    }
    ext_lower[ext_len] = '\0';

    /* Binary search the table */
    const char** found = (const char**)bsearch(
        ext_lower, ext_mime_table,
        sizeof(ext_mime_table) / sizeof(ext_mime_table[0]),
        sizeof(ext_mime_table[0]), mime_ext_compare);

    free(ext_lower);

    if (found) {
        return strdup(found[1]);
    }
    return NULL;
}

#ifndef _NO_PYTHON
static char* python_guess_mime_type(const char* path) {
    PyObject* mimetypes = NULL;
    PyObject* result = NULL;
    char* mime = NULL;

    mimetypes = PyImport_ImportModule("mimetypes");
    if (!mimetypes) {
        PyErr_Clear();
        return NULL;
    }

    result = PyObject_CallMethod(mimetypes, "guess_type", "s", path);
    if (!result || !PyTuple_Check(result) || PyTuple_Size(result) < 1) {
        PyErr_Clear();
        goto cleanup;
    }

    PyObject* mime_obj = PyTuple_GetItem(result, 0);
    if (mime_obj && mime_obj != Py_None && PyUnicode_Check(mime_obj)) {
        const char* mime_str = PyUnicode_AsUTF8(mime_obj);
        if (mime_str) {
            mime = strdup(mime_str);
        }
    }

cleanup:
    Py_XDECREF(result);
    Py_XDECREF(mimetypes);
    return mime;
}
#endif

char* ff_guess_mime_type(const char* path) {
    if (!path) return NULL;

#ifndef _NO_PYTHON
    if (Py_IsInitialized()) {
        char* mime = python_guess_mime_type(path);
        if (mime) return mime;
    }
#endif

    /* Fall back to built-in extension table */
    return mime_from_extension(path);
}
