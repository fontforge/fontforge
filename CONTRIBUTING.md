Contributing to FontForge
=========================

Reporting An Issue
---------------

Use the [GitHub issue tracker](https://github.com/fontforge/fontforge/issues).

First, up in the search box, try searching for similar issues to see if the problem you are facing was reported already. If it was, and your issue is related but not quite the same, please comment on that issue with your own report.If it was not already reported, just open up a new issue. Be sure to include:

* your Operating System and version, 
* your FontForge version and where you got it from, 
* **what happens, step by step, to produce the issue** 
* **what error messages you see,**
* **what you expect to happen**

An easy way to report issues is to record a screencast videos where you explain these things as they happen, and upload it to YouTube and include the YouTube link in your report. 

You can also drag and drop screenshot images into the issue text area from your desktop - and they'll be automatically uploaded and added to the report.

To reproduce the issue, it can helpful to share with the developer community the files you are working with. If you can make a file that is small and only contains what is needed to reproduce the issue, please fork the fontforge repo and add these files to <https://github.com/you/fontforge/tree/master/tests> and submit a pull request. You can also place files on your own website or a file sharing service temporarily (such as MegaUpload, DropBox, Google Drive, etc.) Finally, if you do not wish to make your files publicly available, you can provide an email address for a FontForge developer to contact you at to get a private copy of the file.

Please don't close other people's issues - ask them to close the issue if it is closed to their satisfaction.

### Crash reports

If you are reporting a crash, include a debugger backtrace report.

Contributing Code
------------------------

We request that all pull requests be actively reviewed by at least one other developer, and for 2014 Frank Trampe has volunteered to do that if no one else gets there first. He aims to review your PR within 1 week of your most recent commit.

### How To Contribute, Step by Step

Contribute directly to the codebase using GitHub's Pull Requests. 

- Fork the [FontForge repository](https://github.com/fontforge/fontforge) from GitHub.
- Commit your changes locally using `git`, and push them to your personal fork.
- From the main page of your fork, click on the green “Fork” button in order to submit a Pull
  Request.
- Your pull request will be tested via [Travis CI](https://travis-ci.org/) to automatically indicate that your changes do not prevent compilation. FontForge is a big program, so Travis can easily take over 20 minutes to confirm your changes are buildable. Please be patient. 
- If it reports back that there are problems, you can log into the Travis system and check the log report for your pull request to see what the problem was. If no error is shown, just re-run the Travis test for your pull-request (that failed) to see a fresh report since the last report may be for someone else that did a later pull request, or for mainline code. If you add new code to fix your issue/problem, then take note that you need to check the next pull request in the Travis system. Travis issue numbers are different from GitHub issue numbers.

### Coding Style

Some of these guidelines are not followed in the oldest code in the repository, however their use
is required now that FontForge is not a single-man project anymore.

- Please put only one statement per line. This makes semi-automatic processing and reading of diffs
  much easier.
- For boolean variables, use `stdbool.h`’s `true` and `false`, not an integer.
- Return statements should be inline with the indentation level they are being put on.
- Use POSIX/gnulib APIs in preference to glib, e.g. `strdup` instead of `g_strdup` and `xvasprintf`
  instead of `g_printf_strdup`. This minimizes the impact of non-standard types and functions.

### People To Ask

Various areas of the codebase have been worked on by different people in recent years, so if you are unfamiliar with the general area you're working in, please feel free to chat with people who have experience in that area:

* Build System: Debian - Frank Trampe (frank-trampe)
* Build System: OS X (Applicatoin bundle, Homebrew) - Dr Ben Martin (monkeyiq)
* Build System: Windows - Jeremie Tan (jtanx)
* Feature: Collab - Dr Ben Martin
* Feature: Multi Glyph CharView - Dr Ben Martin
* Feature: UFO import/export - Frank Trampe (frank-trampe)
* Crashes: Frank Trampe, Adrien Tetar (adrientetar)
