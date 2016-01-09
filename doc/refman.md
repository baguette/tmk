# TMake 0.1 alpha Reference Manual

Copyright (c) 2016 Cory Burgett

TMake is a build automation tool intended to be a portable, flexible, and familiar replacement for UNIX `make`, providing the full power of a general-purpose scripting language (Tcl).  The TMake distribution is completely self-contained:  the only prerequisite is an ANSI C compiler.  TMake was designed and developed by Cory Burgett and Andre van Schalkwyk, and released under the 2-clause BSD license.


## Terminology

This reference manual makes use of the following terms:

* **target** - The name of the product of a TMake execution step, or the name of a file that already exists in the file system.
* **construct** - To perform the steps necessary to update a target (e.g., by evaluating the rules associated with that target).
* **goal** - The top-level target that TMake is constructing. The goal is either the default target (the target of the first rule defined in the file) or the target specified on the command line.
* **dependency** - A target that must be constructed prior to any target that depends on it.
* **recipe** - A Tcl sub-script that is executed in order to construct a target.
* **rule** - A triple of (target-list, dependency-list, recipe) that defines how to construct one or more targets provided that the dependencies have been constructed.  The targets are constructed by evaluating the associated recipe.  The recipe is optional, in which case the target is considered up to date if all of its dependencies are up to date, and no further action is taken to update the target.
* **parameter** - A variable holding a value that may be overridden on the command line or from the environment.
* **TMakefile** - A Tcl script that defines rules and parameters necessary to construct a project.
* **command** or **procedure** - A unit of execution that performs a small task when supplied with required information (arguments).
* **variable** - A named storage location containing a value.
* **rule variable** - While TMakefile authors are free to define new variables, several variables are always available within a recipe:  TARGET (the target of the rule), INPUTS (the list of dependencies), and OODATE (a list of only those dependencies which have been deemed out of date).  These variables are known as *rule variables*.
* **explicit and implicit rules** - All rules defined by the `rule` command are *explicit*. Some commands (such as `pattern`) define *implicit* rules. The only difference is in how TMake determines which rule to use to construct a given target. When looking for a rule to construct a target, TMake first looks for an explicit rule that matches the target. If an explicit rule is found, TMake stops the search and uses it to construct the target, or else TMake continues by looking for an implicit rule.  If an implicit rule is found, TMake stops the search and uses it to construct the target, or else TMake continues by looking for a file whose name matches the target.  If no such file is found, TMake reports an error.  Any given target may have more than one rule associated with it, but only the first rule found will be used.


## Example

------------------------------------------------------------------------------
    param CC = gcc
    rule all {foo}
    rule foo.o {foo.c} {
		exec $CC -o $TARGET -c $INPUTS
	}
	rule foo {foo.o} {
		exec $CC -o TARGET $INPUTS
	}
------------------------------------------------------------------------------

* The `param` command defines a parameter named `CC` which defaults to `gcc`, but can be overridden by the user on the command line or from the environment (if -e is specified).
* The first rule defines a target named `all` which depends on the `foo` target.  The rule for `all` does not include a recipe. Since it is the first rule defined in the TMakefile, `all` is the default target, and therefore the goal, of this TMakefile.
* The second rule defines a target named `foo.o` which depends on `foo.c`.  In this case, since there is no rule defined for `foo.c`, TMake assumes it is a filename.  This rule uses a recipe to compile `foo.c` into `foo.o`, using the `CC` parameter and the rule variables `TARGET` and `INPUTS`.
* The third and final rule defines a target named `foo` which depends on `foo.o`. This rule uses a recipe to link foo.o into an executable named `foo`.
* When TMake evaluates this TMakefile, it starts with the default goal and determines the prerequisite dependencies.  It also determines the order in which the rules must be applied in order to ensure that all dependencies have been satisfied.  Using the dependency information, it determines that the rules must be applied in this order:  `foo.o`, `foo`, `all`.


## Command line options

* `-n`: Display the commands that *would* be executed to construct a target without actually executing them.
* `-f ` *`file`*: Evaluate *`file`* rather than the default *`TMakefile`*.
* `-I ` *`dir`*: Specify an additional directory to be searched for include files.  May be specified more than once.
* `-P ` *`dir`*: Specify an additional directory to be searched for package files.  May be specified more than once.
* `-D ` *`param`*: Define a parameter *`param`* on the command line (i.e., set it to 1). May be specified more than once.
* `-V ` *`var`*: Display TMake's idea of the value of a variable *`var`* without executing any rules.  May be specified more than once.
* `-j` *`max_processes`*: Evaluate the TMakefile by spawning a number of processes equal to *`max_processes`* (a positive integer).  Defaults to 1.
* `-e`: Use environment variables to override parameters defined in the TMakefile.
* `-u`: Construct the goal even if it is up to date.
* `-s`: Silent mode:  do not display commands that are executed while processing the TMakefile.


## Command Reference

This section covers commands provided by TMake.  Note that all commands implemented by [Jim Tcl](http://jim.tcl.tk/) are also available in TMakefiles.  Jim Tcl commands are not covered here;  for a reference of Jim Tcl commands see [the Jim Tcl User Reference Manual](http://jim.tcl.tk/fossil/doc/trunk/Tcl_shipped.html).

### rule

**`rule `** *`target-list dependency-list ?recipe?`*

Define an explicit rule for one or more targets.  The first rule defined in a TMakefile specifies the default target.  Multiple targets are handled independently from one another. That is, specifying multiple targets for a single rule is simply a shorthand for specifying two rules that have the same dependencies and recipe, but different targets.

The following rule variables are available in recipes:

* `TARGET`: The name of the target.
* `INPUTS`: The full list of target dependencies.
* `OODATE`: The list of dependencies that have been deemed out of date by TMake.

### param

**`param `** *`name = default-value`*

Define a parameter with a default value that may be overridden on the command line or from the environment.  When initializing a parameter, the value is first searched for on the command line, then in the environment (if `-e` was specified), then finally the *`default-value`*.

### exec

**`exec `** *`?-flags flags? arg ...`*

Without any flags, `exec` first displays its arguments, then issues the arguments as a shell command.  If the command exits with a failure condition, TMake stops executing the TMakefile and reports an error.If `-n` is specified on the command line, the arguments are only displayed, not executed.

Flags alter the behavior of `exec`.  *`flags`* may be any combination of the following:

* `@`: "Silence".  Execute the arguments without displaying them.
* `-`: "Ignore errors".  Do not stop executing the TMakefile, even if the command exited with a failure condition.
* `+`: "Always execute". Execute the command even if `-n` was specified on the command line.  This can be useful for debugging recursive TMakefiles.

**Note**:  This is not the Tcl exec command.  In TMakefiles, the original Tcl exec command is available as `tcl::exec`.

### include

**`include `** *`filename`*

Evaluate a Tcl file named *`filename`*.  TMake uses the include search path to find the file.  It defaults to the current directory and the system-wide TMake include directory, but additional directories can be specified with `-I` on the command line.  The include search path is available in TMakefiles as a list stored in the variable `TM_INCLUDE_PATH`.

### make

**`make `** *`target`*

Returns 1 if *`target`* is the goal of this execution of TMake, or else returns 0.

### defined

**`defined `** *`name`*

Returns 1 if a parameter or variabled called *`name`* has been defined, or else returns 0.

### target

**`target `** *`name`*

Returns 1 if a target called *`name`* has been defined, or else returns 0.

### empty

**`empty `** *`string`*

Returns 1 if *`string`* is the empty string, or else returns 0.

### commands

**`commands `** *`target`*

Returns 1 if the target *`target`* has an associated recipe, or else returns 0.


## Predefined variables

* `TM_CURRENT_GOAL` - The goal of this execution of the TMakefile.
* `TM_PARAM` - An array of parameters that were overridden on the command line.
* `TM_NO_EXECUTE` - Set to 1 if `-n` was specified on the command line.
* `TM_SILENT_MODE` - Set to 1 if `-s` was specified on the command line.
* `TM_ENV_LOOKUP` - Set to 1 if `-e` was specified on the command line.
* `TM_MACHINE_ARCH` - The architecture of the machine `tmake` was built for.

