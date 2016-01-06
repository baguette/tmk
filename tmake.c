#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <jim.h>

int main(int argc, char **argv)
{
	Jim_Interp *interp = NULL;

	/* Create a Tcl interpreter */
	interp = Jim_CreateInterp();
	assert(interp != NULL && "couldn't create interpreter");

	/* Register the core Tcl commands */
	Jim_RegisterCoreCommands(interp);

	/* Initialize any static extensions */
	Jim_InitStaticExtensions(interp);

	/* TMakefile evaluation will go here: */
	Jim_Eval(interp, "puts {Hello world}");

	/* Free the Tcl interpreter */
	Jim_FreeInterp(interp);

	return (EXIT_SUCCESS);
}

