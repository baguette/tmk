
# Check if key is defined as an environment variable
proc env_has {key} {
	global env
	
	if {[string length [array get env $key]]} {
		return 1
	} else {
		return 0
	}
}

# Create a new parameter.  If $var exists as an environment variable,
# the value in the environment is used, or else $val is used.
proc param {var val} {
	global $var
	global env

	if {[env_has $var]} {
		set $var $env($var)
	} else {
		set $var $val
	}
}

set _default_target ""
set _target_dependencies [dict create]
set _target_recipes [dict create]

proc target {name deps recipe} {
	global _default_target
	global _target_dependencies
	global _target_recipes

	if {[dict exists $_target_recipes name]} {
		error "Target $name already exists"
	}

	set _target_recipes [dict set $_target_recipes $name $recipe]
	set _target_dependencies [dict set $_target_dependencies $name $deps]

	if {[string length $_default_target] == 0} {
		set _default_target $name
	}
}

proc end {} {
	global _default_target
	global _target_dependencies
	global _target_recipes

	set recipe [dict get $_target_recipes $_default_target]
	eval $recipe
}

