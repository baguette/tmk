# TMk

A portable `make` replacement powered by Tcl.

<a href="https://scan.coverity.com/projects/tmk">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/7553/badge.svg"/>
</a>


## Powered By

|                                          Component                                    |                                    Description                                      |
|---------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------|
| [Jim Tcl](http://jim.tcl.tk/index.html/doc/www/www/index.html)                        | Jim is an opensource small-footprint implementation of the Tcl programming language |
| [SHA-1](https://github.com/B-Con/crypto-algorithms)                                   | Public Domain implementation by Brad Conte.                                         |
| [SQLite](http://sqlite.org/)                                                          | An embeddable, single-file SQL database.                                            |


## Building & Installing

### UNIX-like systems (including Linux, Mac OS X, and Cygwin)

To build, you'll need an ANSI C compiler and a POSIX-compatible shell.  If you believe you have those things and the build does not work, please file an issue.

1. Use git to fetch the code:

    git clone https://github.com/baguette/tmk.git
    cd tmk

2. Use the provided bootstrap script to build a bootstrap version of `tmk`:

    ./bootstrap.sh

3. Once the bootstrap is done, use `tmk` to build itself:

    ./tmk

4. Optionally, build the documentation (you'll need [pandoc](http://pandoc.org/)):

    ./tmk doc-html    # for HTML documentation
    ./tmk doc-pdf     # for PDF documentation
    ./tmk doc-all     # for both

5. To install TMk, use the `install` target provided in the TMakefile.  Some helpful parameters are `PREFIX` (defaults to `/usr/local`) and (for package maintainers) `DESTDIR` (defaults to the root directory):

    ./tmk PREFIX=/usr install

6. To install the documentation (to `$PREFIX/share/doc/tmk`):

    ./tmk PREFIX=/usr install-doc


### Windows

*Coming soon...*


## Syntax highlighting

TMakefile syntax highlighting support is provided for Vim in the `vim-syntax` directory.

