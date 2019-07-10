/* Copyright (C) 2019 by Jeremy Tan */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * A test driver for the system tests that FontForge has.
 * Runs each test in its own directory, clearing it out first if it exists.
 * Tests will be skipped (return code 77) if required inputs are missing.
 */

#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include <stdio.h>

#ifdef G_OS_UNIX
#include <sys/types.h>
#include <sys/wait.h>
#endif

typedef struct ArgData {
    gchar *mode;
    gchar *binary;
    gchar *script;
    gchar *desc;
    gchar *exedir;
    gchar *libdir;
    gchar **argdirs;
    gchar *workdir; // this is computed internally
    gboolean skip_as_pass;
} ArgData;

static void make_absolute(gchar** arg);
static gboolean resolve_args(ArgData *args, gchar **argv);
static gboolean setup_test_dir(ArgData *args);
static int run_ff_systest(ArgData *args, gchar **argv);
static int run_pyhook_systest(ArgData *args, gchar **argv);

int main(int argc, char *argv[]) {
    ArgData args = {0};
    gchar **parsed_argv, **additional_args;
    GOptionContext *context;
    GOptionGroup *group;
    GError *error = NULL;
    int retcode = 0;

    const GOptionEntry entries[] = {
        {"mode", 'm', 0, G_OPTION_ARG_STRING, &args.mode, "The mode to run in; required", "<ff|py|pyhook>"},
        {"binary", 'b', 0, G_OPTION_ARG_FILENAME, &args.binary, "The path to the executable; required", NULL},
        {"script", 'c', 0, G_OPTION_ARG_FILENAME, &args.script, "The path to the test script; required", NULL},
        {"desc", 'd', 0, G_OPTION_ARG_FILENAME, &args.desc, "The test description; required", NULL},
        {"exedir", 'e', 0, G_OPTION_ARG_FILENAME, &args.exedir, "The path to the directory containing the built executables; required", NULL},
        {"libdir", 'l', 0, G_OPTION_ARG_FILENAME, &args.libdir, "The path the to directory containing the built libraries; required", NULL},
        {"argdir", 'a', 0, G_OPTION_ARG_FILENAME_ARRAY, &args.argdirs, "The path used to resolve test arguments; required", NULL},
        {"skip-as-pass", 's', 0, G_OPTION_ARG_NONE, &args.skip_as_pass, "Exit with a return code of 0 instead of 77 for skipped tests", NULL},
        {0},
    };

#if !(GLIB_CHECK_VERSION(2, 35, 0))
    g_type_init();
#endif

    // The suggested usage leaks memory...
#ifdef G_OS_WIN32
    parsed_argv = g_win32_get_command_line();
#else
    parsed_argv = g_strdupv(argv);
#endif

    context = g_option_context_new("- system test driver for FontForge");
    group = g_option_group_new("", "", "", &args, NULL);
    g_option_context_set_main_group(context, group);
    g_option_group_add_entries(group, entries);

    if (!g_option_context_parse(context, &argc, &parsed_argv, &error)) {
        if (error) {
            fprintf(stderr, "%s\n", error->message);
        } else {
            fprintf(stderr, "failed to parse command line\n");
        }
        return 1;
    }

    // Skip the first argument; will be the name of this executable
    additional_args = parsed_argv + 1;

    make_absolute(&args.script);
    make_absolute(&args.exedir);
    make_absolute(&args.libdir);

    if (!args.mode || !args.binary || !args.script || !args.exedir || !args.libdir || !args.argdirs) {
        fprintf(stderr, "missing one or more required arguments\n");
        retcode = 1;
    } else if (strcmp(args.mode, "ff") && strcmp(args.mode, "py") && strcmp(args.mode, "pyhook")) {
        fprintf(stderr, "unknown mode '%s'\n", args.mode);
        retcode = 1;
    } else {
        fprintf(stderr, "Test %s(mode=%s): %s\n", args.script, args.mode, args.desc);
        if (!resolve_args(&args, additional_args)) {
            fprintf(stderr, "Test %s skipped as a required file is missing!\n", args.script);
            retcode = args.skip_as_pass ? 0 : 77;
        } else if (!setup_test_dir(&args)) {
            fprintf(stderr, "Test %s errored as test directory could not be set up\n", args.script);
            retcode = 1;
        } else {
            if (!strcmp(args.mode, "pyhook")) {
                retcode = run_pyhook_systest(&args, additional_args);
            } else {
                retcode = run_ff_systest(&args, additional_args);
            }
            fprintf(stderr, "Test %s %s with exit code %d\n", args.script,
                retcode == 0 ? "passed" :
                retcode == 77 ? "skipped" : "failed", retcode);
        }
    }

    g_option_context_free(context);
    g_strfreev(parsed_argv);

    g_free(args.mode);
    g_free(args.binary);
    g_free(args.script);
    g_free(args.desc);
    g_free(args.exedir);
    g_free(args.libdir);
    g_free(args.workdir);
    g_strfreev(args.argdirs);

    return retcode;
}

static void make_absolute(gchar** arg) {
    if (*arg && !g_path_is_absolute(*arg)) {
        gchar *pwd = g_get_current_dir();
        gchar *newpath = g_build_filename(pwd, *arg, NULL);
        g_free(*arg);
        g_free(pwd);
        *arg = newpath;
    }
}

static gboolean resolve_args(ArgData *args, gchar **argv) {
    size_t i,j;

    for (i = 0; args->argdirs[i]; i++) {
        make_absolute(&args->argdirs[i]);
    }

    for (i = 0; argv[i]; ++i) {
        gboolean found = FALSE;

        for (j = 0; args->argdirs[j]; ++j) {
            gchar *path = g_build_filename(args->argdirs[j], argv[i], NULL);

            if (g_file_test(path, G_FILE_TEST_EXISTS)) {
                found = TRUE;
                g_free(argv[i]);
                argv[i] = path;
                break;
            }
            g_free(path);
        }

        if (!found) {
            fprintf(stderr, "could not resolve the location to %s\n", argv[i]);
            return FALSE;
        }
    }

    return TRUE;
}

static gboolean remove_dir(const char *path) {
    if (g_remove(path) != 0) {
        if (g_file_test(path, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_DIR)) {
            GDir *dir;
            const gchar *entry;
            gboolean cont = TRUE;

            dir = g_dir_open(path, 0, NULL);
            if (dir == NULL) {
                fprintf(stderr, "Failed to diropen %s\n", path);
                return FALSE;
            }
            while (cont && (entry = g_dir_read_name(dir))) {
                gchar *fpath = g_build_filename(path, entry, NULL);
                if (g_remove(fpath) != 0 && g_file_test(fpath, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_DIR)) {
                    cont = remove_dir(fpath);
                }
                g_free(fpath);
            }
            g_dir_close(dir);
        }
        return g_remove(path) == 0 || !g_file_test(path, G_FILE_TEST_EXISTS);
    }
    return TRUE;
}

static gboolean setup_test_dir(ArgData *args) {
    gchar *name, *dir_name;

    name = dir_name = g_path_get_basename(args->script);
    for (; *name; ++name) {
        if (!g_ascii_isalnum(*name)) {
            *name = '_';
        }
    }

    name = dir_name;
    dir_name = g_strconcat(name, "_", args->mode, NULL);
    g_free(name);
    name = g_build_filename("systests", dir_name, NULL);
    g_free(dir_name);

    if (!remove_dir(name) || g_mkdir_with_parents(name, 0755)) {
        fprintf(stderr, "failed to setup the working directory: %s\n", name);
        g_free(name);
        return FALSE;
    }

    args->workdir = name;
    return TRUE;
}

static int run_executable(ArgData *args, gchar **argv) {
    int retcode = 0;
    gboolean ret;
    GError *error = NULL;
    gchar *binary, *tmp;
    
    binary = g_find_program_in_path(argv[0]);
    make_absolute(&binary); // ^ is broken?!
    if (!binary) {
        fprintf(stderr, "failed to find %s\n", argv[0]);
        return 1;
    }
    tmp = argv[0];
    argv[0] = binary;
    binary = tmp;
    
    tmp = g_strjoinv(" ", argv);
    fprintf(stderr, "Running: %s\n", tmp);
    fflush(stderr);
    g_free(tmp);

    ret = g_spawn_sync(args->workdir, argv, NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, NULL, &retcode, &error);
    g_free(argv[0]);
    argv[0] = binary;

    if (!ret) {
         fprintf(stderr, "execution failed: %d: %s\n", retcode, error ? error->message : "unknown error");
         g_error_free(error);
         retcode = 1;
    }
#ifdef G_OS_UNIX
    else {
        if (WIFEXITED(retcode)) {
            retcode = WEXITSTATUS(retcode);
        } else {
            fprintf(stderr, "unknown waitpid status %d\n", retcode);
            retcode = 1;
        }
    }
#endif
    return retcode;
}

static int run_ff_systest(ArgData *args, gchar **argv) {
    GPtrArray *test_args = g_ptr_array_new();
    int retcode;

    g_ptr_array_add(test_args, args->binary);
    g_ptr_array_add(test_args, "-lang");
    g_ptr_array_add(test_args, args->mode);
    g_ptr_array_add(test_args, "-script");
    g_ptr_array_add(test_args, args->script);
    while (*argv) {
        g_ptr_array_add(test_args, *argv++);
    }
    g_ptr_array_add(test_args, NULL);

    retcode = run_executable(args, (gchar**)test_args->pdata);
    g_ptr_array_free(test_args, TRUE);
    return retcode;
}

static int run_pyhook_systest(ArgData *args, gchar **argv) {
    GPtrArray *test_args = g_ptr_array_new();
    int retcode;

#ifdef G_OS_WIN32
    char *path = g_strconcat(args->exedir, G_SEARCHPATH_SEPARATOR_S, g_getenv("PATH"), NULL);
    g_setenv("PATH", path, TRUE);
    g_free(path);
#endif

    g_setenv("PYTHONPATH", args->libdir, TRUE);

    g_ptr_array_add(test_args, args->binary);
    g_ptr_array_add(test_args, "-Ss");
    g_ptr_array_add(test_args, args->script);
    while (*argv) {
        g_ptr_array_add(test_args, *argv++);
    }
    g_ptr_array_add(test_args, NULL);

    retcode = run_executable(args, (gchar**)test_args->pdata);
    g_ptr_array_free(test_args, TRUE);
    return retcode;
}
