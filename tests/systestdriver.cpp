/* Copyright (C) 2019 by Jeremy Tan */
/* Copyright (C) 2025 FontForge Contributors - C++ port */
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
 *
 * This is a standalone tool with no FontForge library dependencies.
 */

#include <cxxopts.hpp>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#define PATH_SEP '\\'
#define PATH_SEP_STR "\\"
#define stat _stat
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#define access _access
#define F_OK 0
#define getcwd _getcwd
#define chdir _chdir
#define rmdir _rmdir
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define PATH_SEP '/'
#define PATH_SEP_STR "/"
#endif

struct ArgData {
    std::string mode;
    std::string binary;
    std::string script;
    std::string desc;
    std::string exedir;
    std::string libdir;
    std::vector<std::string> argdirs;
    std::string workdir;
    bool skip_as_pass = false;
};

/* ==================== File System Utilities ==================== */

static bool is_absolute(const std::string& path) {
    if (path.empty()) return false;
#ifdef _WIN32
    // Check for drive letter (C:) or UNC path (\\)
    if (path.size() >= 2) {
        if (path[1] == ':') return true;
        if (path[0] == '\\' && path[1] == '\\') return true;
    }
    return path[0] == '/' || path[0] == '\\';
#else
    return path[0] == '/';
#endif
}

static std::string get_cwd() {
    char buf[4096];
    if (getcwd(buf, sizeof(buf))) {
        return std::string(buf);
    }
    return ".";
}

static std::string make_absolute(const std::string& path) {
    if (path.empty()) return path;
    if (is_absolute(path)) return path;

    std::string cwd = get_cwd();
    if (cwd.empty() || cwd == ".") return path;

    char sep = cwd.back();
    if (sep == '/' || sep == '\\') {
        return cwd + path;
    }
    return cwd + PATH_SEP_STR + path;
}

static std::string path_join(const std::string& dir, const std::string& name) {
    if (dir.empty()) return name;
    if (name.empty()) return dir;

    char sep = dir.back();
    if (sep == '/' || sep == '\\') {
        return dir + name;
    }
    return dir + PATH_SEP_STR + name;
}

static std::string get_basename(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

static bool file_exists(const std::string& path) {
    return access(path.c_str(), F_OK) == 0;
}

static bool is_directory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

static bool set_env(const char* name, const char* value) {
#ifdef _WIN32
    return _putenv_s(name, value) == 0;
#else
    return setenv(name, value, 1) == 0;
#endif
}

#ifdef _WIN32
static std::string get_env(const char* name) {
    const char* val = getenv(name);
    return val ? val : "";
}
#endif

static int make_directory(const std::string& path) {
#ifdef _WIN32
    return _mkdir(path.c_str());
#else
    return mkdir(path.c_str(), 0755);
#endif
}

static int mkdir_p(const std::string& path) {
    if (path.empty()) return 0;
    if (is_directory(path)) return 0;

    // Find parent directory
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos && pos > 0) {
        std::string parent = path.substr(0, pos);
        if (!parent.empty() && !is_directory(parent)) {
            if (mkdir_p(parent) != 0) return -1;
        }
    }

    return make_directory(path);
}

static bool remove_file(const std::string& path) {
#ifdef _WIN32
    return DeleteFileA(path.c_str()) != 0;
#else
    return unlink(path.c_str()) == 0;
#endif
}

static bool remove_empty_dir(const std::string& path) {
    return rmdir(path.c_str()) == 0;
}

#ifdef _WIN32

static bool remove_dir_recursive(const std::string& path) {
    if (remove_file(path)) return true;
    if (remove_empty_dir(path)) return true;

    if (!is_directory(path)) {
        return !file_exists(path);
    }

    WIN32_FIND_DATAA ffd;
    std::string search = path + "\\*";
    HANDLE hFind = FindFirstFileA(search.c_str(), &ffd);

    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool ok = true;
    do {
        if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0) {
            continue;
        }
        std::string child = path_join(path, ffd.cFileName);
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            ok = remove_dir_recursive(child) && ok;
        } else {
            ok = remove_file(child) && ok;
        }
    } while (FindNextFileA(hFind, &ffd) != 0);

    FindClose(hFind);
    remove_empty_dir(path);
    return !file_exists(path);
}

#else  // POSIX

static bool remove_dir_recursive(const std::string& path) {
    if (remove_file(path)) return true;
    if (remove_empty_dir(path)) return true;

    if (!is_directory(path)) {
        return !file_exists(path);
    }

    DIR* dir = opendir(path.c_str());
    if (!dir) {
        fprintf(stderr, "Failed to open directory %s\n", path.c_str());
        return false;
    }

    bool ok = true;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        std::string child = path_join(path, entry->d_name);
        if (is_directory(child)) {
            ok = remove_dir_recursive(child) && ok;
        } else {
            ok = remove_file(child) && ok;
        }
    }
    closedir(dir);

    remove_empty_dir(path);
    return !file_exists(path);
}

#endif

/* ==================== Argument Resolution ==================== */

static bool resolve_args(const ArgData& args, std::vector<std::string>& argv) {
    for (auto& arg : argv) {
        bool found = false;
        for (const auto& dir : args.argdirs) {
            std::string path = path_join(dir, arg);
            if (file_exists(path)) {
                arg = path;
                found = true;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "could not resolve the location to %s\n", arg.c_str());
            return false;
        }
    }
    return true;
}

static bool setup_test_dir(ArgData& args) {
    std::string basename = get_basename(args.script);

    // Replace non-alphanumeric with underscore
    for (char& c : basename) {
        if (!isalnum(static_cast<unsigned char>(c))) {
            c = '_';
        }
    }

    std::string dir_name = basename + "_" + args.mode;
    std::string name = path_join("systests", dir_name);

    if (!remove_dir_recursive(name)) {
        fprintf(stderr, "failed to remove existing directory: %s\n", name.c_str());
        return false;
    }

    if (mkdir_p(name) != 0) {
        fprintf(stderr, "failed to create working directory: %s\n", name.c_str());
        return false;
    }

    args.workdir = make_absolute(name);
    return true;
}

/* ==================== Process Execution ==================== */

#ifdef _WIN32

static int run_process(const std::string& workdir, const std::vector<std::string>& argv_vec) {
    // Build command line
    std::string cmdline;
    for (size_t i = 0; i < argv_vec.size(); i++) {
        if (i > 0) cmdline += ' ';
        // Quote arguments containing spaces
        if (argv_vec[i].find(' ') != std::string::npos) {
            cmdline += '"';
            cmdline += argv_vec[i];
            cmdline += '"';
        } else {
            cmdline += argv_vec[i];
        }
    }

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    // CreateProcessA modifies the command line, so we need a mutable copy
    std::vector<char> cmdline_buf(cmdline.begin(), cmdline.end());
    cmdline_buf.push_back('\0');

    if (!CreateProcessA(
            nullptr,
            cmdline_buf.data(),
            nullptr, nullptr,
            FALSE,
            0,
            nullptr,
            workdir.c_str(),
            &si, &pi)) {
        fprintf(stderr, "CreateProcess failed: %lu\n", GetLastError());
        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code = 1;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return static_cast<int>(exit_code);
}

#else  // POSIX

static int run_process(const std::string& workdir, const std::vector<std::string>& argv_vec) {
    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "fork failed\n");
        return 1;
    }

    if (pid == 0) {
        // Child process
        if (chdir(workdir.c_str()) != 0) {
            _exit(127);
        }

        // Build argv for execvp
        std::vector<char*> argv;
        for (const auto& s : argv_vec) {
            argv.push_back(const_cast<char*>(s.c_str()));
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());

        // If we get here, exec failed
        _exit(127);
    }

    // Parent process
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return 1;
}

#endif

static int run_executable(const ArgData& args, const std::vector<std::string>& argv_vec) {
    // Print command
    fprintf(stderr, "Running:");
    for (const auto& s : argv_vec) {
        fprintf(stderr, " %s", s.c_str());
    }
    fprintf(stderr, "\n");
    fflush(stderr);

    return run_process(args.workdir, argv_vec);
}

static int run_ff_systest(const ArgData& args, const std::vector<std::string>& extra_args) {
    std::vector<std::string> test_args;
    test_args.push_back(args.binary);
    test_args.push_back("-lang");
    test_args.push_back(args.mode);
    test_args.push_back("-script");
    test_args.push_back(args.script);
    for (const auto& arg : extra_args) {
        test_args.push_back(arg);
    }

    return run_executable(args, test_args);
}

static int run_pyhook_systest(const ArgData& args, const std::vector<std::string>& extra_args) {
#ifdef _WIN32
    // Add exedir to PATH
    std::string path = args.exedir + ";" + get_env("PATH");
    set_env("PATH", path.c_str());
#endif

    set_env("PYTHONPATH", args.libdir.c_str());

    std::vector<std::string> test_args;
    test_args.push_back(args.binary);
    test_args.push_back("-Ss");
    test_args.push_back(args.script);
    for (const auto& arg : extra_args) {
        test_args.push_back(arg);
    }

    return run_executable(args, test_args);
}

/* ==================== Main ==================== */

int main(int argc, char* argv[]) {
    ArgData args;
    std::vector<std::string> extra_args;

    try {
        cxxopts::Options options("systestdriver", "System test driver for FontForge");

        options.add_options()
            ("m,mode", "The mode to run in (ff|py|pyhook)", cxxopts::value<std::string>())
            ("b,binary", "The path to the executable", cxxopts::value<std::string>())
            ("c,script", "The path to the test script", cxxopts::value<std::string>())
            ("d,desc", "The test description", cxxopts::value<std::string>())
            ("e,exedir", "Directory containing built executables", cxxopts::value<std::string>())
            ("l,libdir", "Directory containing built libraries", cxxopts::value<std::string>())
            ("a,argdir", "Directories to resolve test arguments", cxxopts::value<std::vector<std::string>>())
            ("s,skip-as-pass", "Exit 0 instead of 77 for skipped tests")
            ("h,help", "Print usage")
        ;

        options.allow_unrecognised_options();

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            printf("%s\n", options.help().c_str());
            return 0;
        }

        if (result.count("mode")) args.mode = result["mode"].as<std::string>();
        if (result.count("binary")) args.binary = result["binary"].as<std::string>();
        if (result.count("script")) args.script = result["script"].as<std::string>();
        if (result.count("desc")) args.desc = result["desc"].as<std::string>();
        if (result.count("exedir")) args.exedir = result["exedir"].as<std::string>();
        if (result.count("libdir")) args.libdir = result["libdir"].as<std::string>();
        if (result.count("argdir")) args.argdirs = result["argdir"].as<std::vector<std::string>>();
        args.skip_as_pass = result.count("skip-as-pass") > 0;

        // Collect unrecognized options as extra arguments
        extra_args = result.unmatched();

    } catch (const cxxopts::exceptions::exception& e) {
        fprintf(stderr, "Error parsing options: %s\n", e.what());
        return 1;
    }

    // Make paths absolute
    args.script = make_absolute(args.script);
    args.exedir = make_absolute(args.exedir);
    args.libdir = make_absolute(args.libdir);
    for (auto& dir : args.argdirs) {
        dir = make_absolute(dir);
    }

    // Validate required arguments
    if (args.mode.empty() || args.binary.empty() || args.script.empty() ||
        args.exedir.empty() || args.libdir.empty() || args.argdirs.empty()) {
        fprintf(stderr, "missing one or more required arguments\n");
        return 1;
    }

    if (args.mode != "ff" && args.mode != "py" && args.mode != "pyhook") {
        fprintf(stderr, "unknown mode '%s'\n", args.mode.c_str());
        return 1;
    }

    fprintf(stderr, "Test %s(mode=%s): %s\n", args.script.c_str(), args.mode.c_str(), args.desc.c_str());

    int retcode = 0;

    if (!resolve_args(args, extra_args)) {
        fprintf(stderr, "Test %s skipped as a required file is missing!\n", args.script.c_str());
        retcode = args.skip_as_pass ? 0 : 77;
    } else if (!setup_test_dir(args)) {
        fprintf(stderr, "Test %s errored as test directory could not be set up\n", args.script.c_str());
        retcode = 1;
    } else {
        if (args.mode == "pyhook") {
            retcode = run_pyhook_systest(args, extra_args);
        } else {
            retcode = run_ff_systest(args, extra_args);
        }

        fprintf(stderr, "Test %s %s with exit code %d\n", args.script.c_str(),
            retcode == 0 ? "passed" : retcode == 77 ? "skipped" : "failed", retcode);
    }

    return retcode;
}
