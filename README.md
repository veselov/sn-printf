# sn-printf
Zero-dependency implementation of snprintf functionality

## Motivation

1. Some platforms/environments do not have printf functionality at all. Having printf formatting makes it significantly easier for generating strings compared to other approaches, is well known, and often required when porting.
2. The implementation of printf() often differs between platforms, most notably in supported modifiers, and in handling of `NULL` values. It becomes a burden on the application to handle the edge cases.

## Functionality

This project provides an implementation of a snprintf-like
functionality, similar to what's offered by C11 standard.

### Limitations

* Positional parameters are not supported
* Thousands separator are always in en_US locale (commas)
* Wide types (`l` modifier) are not supported
* Precision width is limited to 256 characters

PRs are welcome for fixing the limitations (or any other purpose).

## Integration

The provided functionality is not published to any
repository, and is only available as source code.

The recommended integration is to create a git submodule
pointing to this repository, with a commit pointer, or a stable
branch pointer, and then include the build of the sn-printf
as part of the project build.

### C/C++ projects

It's recommended to wrap sn-printf in additional functionality
to additionally expose `printf()` or `asprintf()` functionality; both
are generally more useful than bare `snprintf()`. sn-printf does
not provide memory management, or support for writing into 
file descriptors or `FILE*` constructs, but does provide a generic
buffer-based write implementation.

#### `printf()` example

#### `asprintf()` example

### Other projects

This hasn't been attempted by the maintainer, but it should be
possible to integrate this into any project that has frameworks/support
for including native code.
