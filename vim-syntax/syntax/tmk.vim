" Vim syntax file
" Language:	TMk
" Maintainer:   Cory Burgett <cmburget@gmail.com>
" Based on Tcl/Tk Maintained by:
"		Taylor Venable <taylor@metasyntax.net>
" 		(previously Brett Cannon <brett@python.org>)
" 		(previously Dean Copsey <copsey@cs.ucdavis.edu>)
"		(previously Matt Neumann <mattneu@purpleturtle.com>)
"		(previously Allan Kelly <allan@fruitloaf.co.uk>)
" Original:	Robin Becker <robin@jessikat.demon.co.uk>
" Last Change:	2016/01/08 21:25
" Version:	1.0
" URL:		https://github.com/baguette/tmk
"
" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Basic Tcl commands: http://www.tcl.tk/man/tcl8.5/tclCmd/contents.htm
syn keyword tmkCommand		after append apply array bgerror binary catch cd chan clock
syn keyword tmkCommand		close concat dde dict encoding eof error eval exec exit
syn keyword tmkCommand		expr fblocked fconfigure fcopy file fileevent filename flush
syn keyword tmkCommand		format gets glob global history incr info interp join
syn keyword tmkCommand		lappend lassign lindex linsert list llength load lrange lrepeat
syn keyword tmkCommand		lreplace lreverse lsearch lset lsort memory namespace open package
syn keyword tmkCommand		pid proc puts pwd read regexp registry regsub rename return
syn keyword tmkCommand		scan seek set socket source split string subst tell time
syn keyword tmkCommand		trace unknown unload unset update uplevel upvar variable vwait

" TMk-specific commands
syn keyword tmkCommand		rule param target make commands exists defined
syn keyword tmkCommand		empty tcl::exec include sub replace-ext

" The 'tcl Standard Library' commands: http://www.tcl.tk/man/tcl8.5/tclCmd/library.htm
syn keyword tmkCommand		auto_execok auto_import auto_load auto_mkindex auto_mkindex_old
syn keyword tmkCommand		auto_qualify auto_reset parray tcl_endOfWord tcl_findLibrary
syn keyword tmkCommand		tcl_startOfNextWord tcl_startOfPreviousWord tcl_wordBreakAfter
syn keyword tmkCommand		tcl_wordBreakBefore

" Commands that were added in tcl 8.6

syn keyword tmkCommand		coroutine tailcall throw yield

" Global variables used by tcl: http://www.tcl.tk/man/tcl8.5/tclCmd/tclvars.htm
syn keyword tmkVars		env errorCode errorInfo tmk_library tmk_patchLevel tmk_pkgPath
syn keyword tmkVars		tmk_platform tmk_precision tmk_rcFileName tmk_traceCompile
syn keyword tmkVars		tmk_traceExec tmk_wordchars tmk_nonwordchars tmk_version argc argv
syn keyword tmkVars		argv0 tmk_interactive geometry

" Additional variables used by TMk
syn keyword tmkVars		TARGET INPUTS OODATE TM_PARAM TM_INCLUDE_PATH TM_CURRENT_GOAL

" Strings which expr accepts as boolean values, aside from zero / non-zero.
syn keyword tmkBoolean		true false on off yes no

syn keyword tmkLabel		case default
syn keyword tmkConditional	if then else elseif switch
syn keyword tmkConditional	try finally
syn keyword tmkRepeat		while for foreach break continue

" variable reference
	" ::optional::namespaces
syn match tmkVarRef "$\(\(::\)\?\([[:alnum:]_]*::\)*\)\a[[:alnum:]_]*"
	" ${...} may contain any character except '}'
syn match tmkVarRef "${[^}]*}"

" The syntactic unquote-splicing replacement for [expand].
syn match tmkExpand '\s{\*}'
syn match tmkExpand '^{\*}'


" NAMESPACE
" commands associated with namespace
syn keyword tmkNamespaceSwitch contained children code current delete eval
syn keyword tmkNamespaceSwitch contained export forget import inscope origin
syn keyword tmkNamespaceSwitch contained parent qualifiers tail which command variable
syn region tmkCommand matchgroup=tmkCommandColor start="\<namespace\>" matchgroup=NONE skip="^\s*$" end="{\|}\|]\|\"\|[^\\]*\s*$"me=e-1  contains=tmkLineContinue,tmkNamespaceSwitch

" EXPR
" commands associated with expr
syn keyword tmkMaths contained	abs acos asin atan atan2 bool ceil cos cosh double entier
syn keyword tmkMaths contained	exp floor fmod hypot int isqrt log log10 max min pow rand
syn keyword tmkMaths contained	round sin sinh sqrt srand tan tanh wide

syn region tmkCommand matchgroup=tmkCommandColor start="\<expr\>" matchgroup=NONE skip="^\s*$" end="]\|[^\\]*\s*$"me=e-1  contains=tmkLineContinue,tmkMaths,tmkNumber,tmkVarRef,tmkString,tmktlWidgetSwitch,tmkCommand,tmkPackConf

" format
syn region tmkCommand matchgroup=tmkCommandColor start="\<format\>" matchgroup=NONE skip="^\s*$" end="]\|[^\\]*\s*$"me=e-1  contains=tmkLineContinue,tmkMaths,tmkNumber,tmkVarRef,tmkString,tmktlWidgetSwitch,tmkCommand,tmkPackConf

" PACK
" commands associated with pack
syn keyword tmkPackSwitch	contained	forget info propogate slaves
syn keyword tmkPackConfSwitch	contained	after anchor before expand fill in ipadx ipady padx pady side
syn region tmkCommand matchgroup=tmkCommandColor start="\<pack\>" matchgroup=NONE skip="^\s*$" end="]\|[^\\]*\s*$"he=e-1  contains=tmkLineContinue,tmkPackSwitch,tmkPackConf,tmkPackConfSwitch,tmkNumber,tmkVarRef,tmkString,tmkCommand keepend

" STRING
" commands associated with string
syn keyword tmkStringSwitch	contained	compare first index last length match range tolower toupper trim trimleft trimright wordstart wordend
syn region tmkCommand matchgroup=tmkCommandColor start="\<string\>" matchgroup=NONE skip="^\s*$" end="]\|[^\\]*\s*$"he=e-1  contains=tmkLineContinue,tmkStringSwitch,tmkNumber,tmkVarRef,tmkString,tmkCommand

" ARRAY
" commands associated with array
syn keyword tmkArraySwitch	contained	anymore donesearch exists get names nextelement size startsearch set
" match from command name to ] or EOL
syn region tmkCommand matchgroup=tmkCommandColor start="\<array\>" matchgroup=NONE skip="^\s*$" end="]\|[^\\]*\s*$"he=e-1  contains=tmkLineContinue,tmkArraySwitch,tmkNumber,tmkVarRef,tmkString,tmkCommand

" LSORT
" switches for lsort
syn keyword tmkLsortSwitch	contained	ascii dictionary integer real command increasing decreasing index
" match from command name to ] or EOL
syn region tmkCommand matchgroup=tmkCommandColor start="\<lsort\>" matchgroup=NONE skip="^\s*$" end="]\|[^\\]*\s*$"he=e-1  contains=tmkLineContinue,tmkLsortSwitch,tmkNumber,tmkVarRef,tmkString,tmkCommand

syn keyword tmkTodo contained	TODO

" Sequences which are backslash-escaped: http://www.tmk.tk/man/tmk8.5/tmkCmd/tmk.htm#M16
" Octal, hexadecimal, unicode codepoints, and the classics.
" tmk takes as many valid characters in a row as it can, so \xAZ in a string is newline followed by 'Z'.
syn match   tmkSpecial contained '\\\([0-7]\{1,3}\|x\x\{1,2}\|u\x\{1,4}\|[abfnrtv]\)'
syn match   tmkSpecial contained '\\[\[\]\{\}\"\$]'

" Command appearing inside another command or inside a string.
syn region tmkEmbeddedStatement	start='\[' end='\]' contained contains=tmkCommand,tmkNumber,tmkLineContinue,tmkString,tmkVarRef,tmkEmbeddedStatement
" A string needs the skip argument as it may legitimately contain \".
" Match at start of line
syn region  tmkString		  start=+^"+ end=+"+ contains=tmkSpecial skip=+\\\\\|\\"+
"Match all other legal strings.
syn region  tmkString		  start=+[^\\]"+ms=s+1  end=+"+ contains=tmkSpecial,tmkVarRef,tmkEmbeddedStatement skip=+\\\\\|\\"+

" Line continuation is backslash immediately followed by newline.
syn match tmkLineContinue '\\$'

if exists('g:tmk_warn_continuation')
    syn match tmkNotLineContinue '\\\s\+$'
endif

"integer number, or floating point number without a dot and with "f".
syn case ignore
syn match  tmkNumber		"\<\d\+\(u\=l\=\|lu\|f\)\>"
"floating point number, with dot, optional exponent
syn match  tmkNumber		"\<\d\+\.\d*\(e[-+]\=\d\+\)\=[fl]\=\>"
"floating point number, starting with a dot, optional exponent
syn match  tmkNumber		"\.\d\+\(e[-+]\=\d\+\)\=[fl]\=\>"
"floating point number, without dot, with exponent
syn match  tmkNumber		"\<\d\+e[-+]\=\d\+[fl]\=\>"
"hex number
syn match  tmkNumber		"0x[0-9a-f]\+\(u\=l\=\|lu\)\>"
"syn match  tmkIdentifier	"\<[a-z_][a-z0-9_]*\>"
syn case match

syn region  tmkComment		start="^\s*\#" skip="\\$" end="$" contains=tmkTodo
syn region  tmkComment		start=/;\s*\#/hs=s+1 skip="\\$" end="$" contains=tmkTodo

"syn sync ccomment tmkComment

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_tmk_syntax_inits")
  if version < 508
    let did_tmk_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink tmkSwitch		Special
  HiLink tmkExpand		Special
  HiLink tmkLabel		Label
  HiLink tmkConditional		Conditional
  HiLink tmkRepeat		Repeat
  HiLink tmkNumber		Number
  HiLink tmkError		Error
  HiLink tmkCommand		Statement
  HiLink tmkString		String
  HiLink tmkComment		Comment
  HiLink tmkSpecial		Special
  HiLink tmkTodo		Todo
  " Below here are the commands and their options.
  HiLink tmkCommandColor	Statement
  HiLink tmkLineContinue	WarningMsg
if exists('g:tmk_warn_continuation')
  HiLink tmkNotLineContinue	ErrorMsg
endif
  HiLink tmkStringSwitch	Special
  HiLink tmkArraySwitch	Special
  HiLink tmkLsortSwitch	Special
  HiLink tmkPackSwitch	Special
  HiLink tmkPackConfSwitch	Special
  HiLink tmkMaths		Special
  HiLink tmkNamespaceSwitch	Special
  HiLink tmkWidgetSwitch	Special
  HiLink tmkPackConfColor	Identifier
  HiLink tmkVarRef		Identifier

  delcommand HiLink
endif

let b:current_syntax = "tmk"

" vim: ts=8 noet
