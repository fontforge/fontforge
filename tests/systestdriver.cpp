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
#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;

struct ArgData {
    std::string mode;
    std::string binary;
    fs::path script;
    std::string desc;
    fs::path exedir;
    fs::path libdir;
    std::vector<fs::path> argdirs;
    fs::path workdir;
    bool skip_as_pass = false;
};

/* ==================== Environment Utilities ==================== */

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

/* ==================== Argument Resolution ==================== */

static bool resolve_args(const ArgData& args, std::vector<std::string>& argv) {
    for (auto& arg : argv) {
        bool found = false;
        for (const auto& dir : args.argdirs) {
            fs::path path = dir / arg;
            if (fs::exists(path)) {
                arg = path.string();
                found = true;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "could not resolve the location to %s\n",
                    arg.c_str());
            return false;
        }
    }
    return true;
}

static bool setup_test_dir(ArgData& args) {
    std::string basename = args.script.filename().string();

    // Replace non-alphanumeric with underscore
    for (char& c : basename) {
        if (!isalnum(static_cast<unsigned char>(c))) {
            c = '_';
        }
    }

    std::string dir_name = basename + "_" + args.mode;
    fs::path name = fs::path("systests") / dir_name;

    std::error_code ec;
    fs::remove_all(name, ec);
    if (ec) {
        fprintf(stderr, "failed to remove existing directory: %s (%s)\n",
                name.string().c_str(), ec.message().c_str());
        return false;
    }

    if (!fs::create_directories(name, ec) && ec) {
        fprintf(stderr, "failed to create working directory: %s (%s)\n",
                name.string().c_str(), ec.message().c_str());
        return false;
    }

    args.workdir = fs::absolute(name);
    return true;
}

/* ==================== Process Execution ==================== */

#ifdef _WIN32

static int run_process(const std::string& workdir,
                       const std::vector<std::string>& argv_vec) {
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

    if (!CreateProcessA(nullptr, cmdline_buf.data(), nullptr, nullptr, FALSE, 0,
                        nullptr, workdir.c_str(), &si, &pi)) {
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

static int run_process(const std::string& workdir,
                       const std::vector<std::string>& argv_vec) {
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

static int run_executable(const ArgData& args,
                          const std::vector<std::string>& argv_vec) {
    // Print command
    fprintf(stderr, "Running:");
    for (const auto& s : argv_vec) {
        fprintf(stderr, " %s", s.c_str());
    }
    fprintf(stderr, "\n");
    fflush(stderr);

    return run_process(args.workdir.string(), argv_vec);
}

static int run_ff_systest(const ArgData& args,
                          const std::vector<std::string>& extra_args) {
    std::vector<std::string> test_args;
    test_args.push_back(args.binary);
    test_args.push_back("-lang");
    test_args.push_back(args.mode);
    test_args.push_back("-script");
    test_args.push_back(args.script.string());
    for (const auto& arg : extra_args) {
        test_args.push_back(arg);
    }

    return run_executable(args, test_args);
}

static int run_pyhook_systest(const ArgData& args,
                              const std::vector<std::string>& extra_args) {
#ifdef _WIN32
    // Add exedir to PATH
    std::string path = args.exedir.string() + ";" + get_env("PATH");
    set_env("PATH", path.c_str());
#endif

    set_env("PYTHONPATH", args.libdir.string().c_str());

    std::vector<std::string> test_args;
    test_args.push_back(args.binary);
    test_args.push_back("-Ss");
    test_args.push_back(args.script.string());
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
        cxxopts::Options options("systestdriver",
                                 "System test driver for FontForge");

        options.add_options()("m,mode", "The mode to run in (ff|py|pyhook)",
                              cxxopts::value<std::string>())(
            "b,binary", "The path to the executable",
            cxxopts::value<std::string>())("c,script",
                                           "The path to the test script",
                                           cxxopts::value<std::string>())(
            "d,desc", "The test description", cxxopts::value<std::string>())(
            "e,exedir", "Directory containing built executables",
            cxxopts::value<std::string>())(
            "l,libdir", "Directory containing built libraries",
            cxxopts::value<std::string>())(
            "a,argdir", "Directories to resolve test arguments",
            cxxopts::value<std::vector<std::string>>())(
            "s,skip-as-pass", "Exit 0 instead of 77 for skipped tests")(
            "h,help", "Print usage");

        options.allow_unrecognised_options();

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            printf("%s\n", options.help().c_str());
            return 0;
        }

        if (result.count("mode")) args.mode = result["mode"].as<std::string>();
        if (result.count("binary"))
            args.binary = result["binary"].as<std::string>();
        if (result.count("script"))
            args.script = result["script"].as<std::string>();
        if (result.count("desc")) args.desc = result["desc"].as<std::string>();
        if (result.count("exedir"))
            args.exedir = result["exedir"].as<std::string>();
        if (result.count("libdir"))
            args.libdir = result["libdir"].as<std::string>();
        if (result.count("argdir")) {
            for (const auto& dir :
                 result["argdir"].as<std::vector<std::string>>()) {
                args.argdirs.push_back(fs::path(dir));
            }
        }
        args.skip_as_pass = result.count("skip-as-pass") > 0;

        // Collect unrecognized options as extra arguments
        extra_args = result.unmatched();

    } catch (const cxxopts::exceptions::exception& e) {
        fprintf(stderr, "Error parsing options: %s\n", e.what());
        return 1;
    }

    // Make paths absolute
    if (!args.script.empty()) args.script = fs::absolute(args.script);
    if (!args.exedir.empty()) args.exedir = fs::absolute(args.exedir);
    if (!args.libdir.empty()) args.libdir = fs::absolute(args.libdir);
    for (auto& dir : args.argdirs) {
        dir = fs::absolute(dir);
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

    fprintf(stderr, "Test %s(mode=%s): %s\n", args.script.string().c_str(),
            args.mode.c_str(), args.desc.c_str());

    int retcode = 0;

    if (!resolve_args(args, extra_args)) {
        fprintf(stderr, "Test %s skipped as a required file is missing!\n",
                args.script.string().c_str());
        retcode = args.skip_as_pass ? 0 : 77;
    } else if (!setup_test_dir(args)) {
        fprintf(stderr,
                "Test %s errored as test directory could not be set up\n",
                args.script.string().c_str());
        retcode = 1;
    } else {
        if (args.mode == "pyhook") {
            retcode = run_pyhook_systest(args, extra_args);
        } else {
            retcode = run_ff_systest(args, extra_args);
        }

        fprintf(stderr, "Test %s %s with exit code %d\n",
                args.script.string().c_str(),
                retcode == 0    ? "passed"
                : retcode == 77 ? "skipped"
                                : "failed",
                retcode);
    }

    return retcode;
}
