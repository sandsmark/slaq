# Slaq

Unoffical [Slack](https://slack.com/) client that doesn't eat all your RAM.
Current implementation of Slaq uses 3x less memory for same teams opened

Based on [Slackfish](https://github.com/markussammallahti/harbour-slackfish).

## How to use
Get latest Qt framework. 
Minimum required version is Qt 5.12.4 LTS.
We recommends online installer [here](https://www.qt.io/download-qt-installer)

or download offline versions:
[Linux](http://download.qt.io/official_releases/qt/5.12/5.12.4/qt-opensource-linux-x64-5.12.4.run), [Windows](http://download.qt.io/official_releases/qt/5.12/5.12.4/qt-opensource-windows-x86-5.12.4.exe), [macOS](http://download.qt.io/official_releases/qt/5.12/5.12.4/qt-opensource-mac-x64-5.12.4.dmg)

Platform specifics:

Windows
Windows builds requires Qt 5.12.4 version since it is contains prebuilt OpenSSL libraries. The libraries should be installed during Qt installation process
Built with MinGW 7.3.0

Linux
Minimun GCC version is 5.x (Linux)

macOS
No built image provided yet. WIP


Note: currently tested only on Linux & Windows

Compile, build and run: `qmake && make && ./slaq`

Privacy policy, because that is necessary: We don't connect to anything apart from Slack.

[![Patreon](https://c5.patreon.com/external/logo/become_a_patron_button.png)](https://www.patreon.com/bePatron?u=16963463)

![Alt text](https://user-images.githubusercontent.com/11473810/55072233-d58ce380-508a-11e9-9e6a-b6b80fc83dd6.png?raw=true "Screenshot")
