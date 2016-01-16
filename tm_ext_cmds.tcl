
set TM_PLATFORM "$TM_OPSYS-$TM_MACHINE_ARCH"

# Check if a variable var is defined
proc defined {var} {
	if {[uplevel "info exists $var"]} {
		return 1
	} else {
		return 0
	}
}

# True if target is the current goal
proc make {target} {
	global TM_CURRENT_GOAL

	if {"$target" eq "$TM_CURRENT_GOAL"} {
		return 1
	} else {
		return 0
	}
}

# True if operand is the empty string
proc empty {operand} {
	if {[string length $operand]} {
		return 0
	} else {
		return 1
	}
}

# Check if an array a contains a value for key
proc array_has {a key} {
	if {[string length [uplevel "array get $a $key"]]} {
		return 1
	} else {
		return 0
	}
}

# Set a global parameter var to val using mode mode
proc param {var mode val} {
	global TM_PARAM
	global TM_ENV_LOOKUP
	global env

	if {$mode eq "="} {
		if {[array_has TM_PARAM $var]} {
			uplevel "set $var {$TM_PARAM($var)}"
		} elseif {[info exists TM_ENV_LOOKUP]} {
			uplevel "set $var {$env($var)}"
		} else {
			uplevel "set $var {$val}"
		}
	} else {
		error "Unknown mode in parameter assignment: $mode"
	}
}

# Clean up a list of options to make it suitable for exec
proc options {str} {
	set OPTIONS [split $str "\n"]
	set TRIMOPTS {}
	foreach OPT $OPTIONS {
		set trim [string trim $OPT]
		if {[string length $trim]} {
			lappend TRIMOPTS $trim
		}
	}
	return $TRIMOPTS
}

rename exec tcl::exec

proc exec args {
	global TM_NO_EXECUTE
	global TM_SILENT_MODE
	set flags ""
	set echo 1
	set errexit 1
	set noexec 0
	set start 0

	if {[defined TM_NO_EXECUTE]} {
		set noexec $TM_NO_EXECUTE
	}

	if {[llength args] == 0} {
		return "";
	}

	if {"[lindex $args 0]" eq "-flags"} {
		if {[llength $args] < 2} {
			error "No flags provided to -flags"
		}
		set flags [lindex $args 1]
		set start 2
	}

	for {set i 0} {$i < [string length $flags]} {incr i} {
		switch [string index $flags $i] {
			"@" {set echo 0}
			"-" {set errexit 0}
			"+" {set noexec 0}
			default {error "Unknown flag given to exec: [string index $i]"}
		}
	}

	set rest [lrange $args $start end]
	
	if {$echo && ![defined TM_SILENT_MODE]} {
		# Do it this way to avoid having Tcl escape quotes and such
		# in the output.
		foreach arg [lrange $rest 0 end-1] {
			puts -nonewline "$arg "
		}
		puts "[lrange $rest end end]"
		flush stdout
	}

	if {$noexec} {
		return ""
	}

	set childpid [tcl::exec {*}$rest &]
	set status [os.wait $childpid]    ;# TODO: This might not work on Windows...

	if {"[lindex $status 1]" eq "exit" && [lindex $status 2] != 0} {
		if {$errexit} {
			error "exec returned non-zero return code: $status"
		}
	} elseif {"[lindex $status 1]" ne "exit"} {
		if {$errexit} {
			error "exec terminated abnormally: $status"
		}
	}

	return ""
}


# Take a list of filenames and replace the extensions that match x with y
proc replace-ext {files x y} {
	set ys {}
	foreach f $files {
		if {"[file extension $f]" eq "$x"} {
			lappend ys "[file rootname $f]$y"
		} else {
			lappend ys $f
		}
	}
	return $ys
}


# Define a substitution rule.  Returns a list of all the targets created.
proc sub {from to recipe} {
	set OUT {}

	foreach in [glob *$from] {
		set out [replace-ext $in $from $to]
		rule $out $in $recipe
		lappend OUT $out
	}

	return $OUT
}

