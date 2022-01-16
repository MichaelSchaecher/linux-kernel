## Linux kernel

There are several guides for kernel developers and users. These guides can
be rendered in a number of formats, like HTML and PDF. Please read
Documentation/admin-guide/README.rst first.

In order to build the documentation, use ``make htmldocs`` or
``make pdfdocs``.  The formatted documentation can also be read online at:

<p align="center"><a href="https://www.kernel.org/doc/html/latest/"><b>Kernel Docs</b></a></p>

There are various text files in the Documentation/ subdirectory,
several of them using the Restructured Text markup notation.

Please read the Documentation/process/changes.rst file, as it contains the
requirements for building and running the kernel, and information about
the problems which may result by upgrading your kernel.

## Changes Made for WSL

This is by default the current stable Linux kernel source found **[here](https://www.kernel.org/)**; then only notable changes made is for _WSL2_ support.

Instead of copying or downloading the config file from Microsoft's Linux kernel **[rep](https://github.com/microsoft/WSL2-Linux-Kernel)**, I've added support for x86_64 hardware. This means that if you run any of the `make` commands for configure the build environment, if you was running the Linux distro inside _wsl_ then the right config file well be used.

For more info about how to compile the kernel for _wsl_ visit this how to **[guide](https://michaelschaecher.github.io/2022-01-15-how-to-compile-the-kernel-for-wsl/)**.
