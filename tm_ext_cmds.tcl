proc defined {var} {
	if {[info exists $var]} {
		return 1
	} else {
		return 0
	}
}

proc array_has {a key} {
	global $a

	if {[string length [array get $a $key]]} {
		return 1
	} else {
		return 0
	}
}

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

