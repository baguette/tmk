
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
	global $var
	
	if {$mode == "="} {
		if {[array_has TM_PARAM $var]} {
			set $var $TM_PARAM($var)
		} else {
			set $var $val
		}
	} else {
		error "Unknown mode in parameter assignment: $mode"
	}
}

