Contributing to FontForge
=========================

Reporting An Issue
---------------

Please head over to the [GitHub issue tracker](https://github.com/fontforge/fontforge/issues).

Up in the search box, try to search and see if the problem you are facing was reported already.

- If it was, please put your own report with the FontForge version, symptoms, error message and
  your Operating System (Windows, Mac OS, GNU+Linux, ...). If the current report is similar to
  what you are facing, please bump the issue so that developers are aware that it still warrants
  a fix.

- If it was not, just open up a new issue.

- If you are reporting a crash, include a debugger backtrace report, a file to reproduce the crash with, and step by step instructions (ideally a screencast video) to reproduce it

Thank you!

Contributing Code
------------------------

### Coding Style

Some of these guidelines are not followed in the oldest code in the repository, however their use
is required now that FontForge is not a single-man project anymore.

- Please put only one statement per line. This makes semi-automatic processing and reading of diffs
  much easier.
- For boolean variables, use `stdbool.h`’s `true` and `false`, not an integer.
- Return statements should be inline with the indentation level they are being put on.
- Use POSIX/gnulib APIs in preference to glib, e.g. `strdup` instead of `g_strdup` and `xvasprintf`
  instead of `g_printf_strdup`. This minimizes the impact of non-standard types and functions.

### Submitting a Pull Request

Pull Requests are submitted via GitHub, an online developers’ network.

- Fork the [FontForge repository](https://github.com/fontforge/fontforge) from GitHub.
- Commit your changes locally using `git`, and push them to your personal fork.
- From the main page of your fork, click on the green “Fork” button in order to submit a Pull
  Request.
- Your pull request will be tested via [Travis CI](https://travis-ci.org/) to automatically indicate that your changes do not prevent compilation. FontForge is a big program, so Travis can easily take over 20 minutes to confirm your changes are buildable. Please be patient. 
- If it reports back that there are problems, you can log into the Travis system and check the log report for your pull request to see what the problem was. If no error is shown, just re-run the Travis test for your pull-request (that failed) to see a fresh report since the last report may be for someone else that did a later pull request, or for mainline code. If you add new code to fix your issue/problem, then take note that you need to check the next pull request in the Travis system. Travis issue numbers are different from GitHub issue numbers.
