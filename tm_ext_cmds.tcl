# Check if a variable var is defined
proc defined {var} {
	if {[info exists $var]} {
		return 1
	} else {
		return 0
	}
}

# Check if an array a contains a value for key
proc array_has {a key} {
	global $a

	if {[string length [array get $a $key]]} {
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
	global $var
	
	if {$mode == "="} {
		if {[array_has TM_PARAM $var]} {
			set $var $TM_PARAM($var)
		} elseif {[info exists TM_ENV_LOOKUP]} {
			set $var $env($var)
		} else {
			set $var $val
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

