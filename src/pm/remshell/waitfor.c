/* 
 * This routine can be called to handle the result of a wait.  
 * This is in a separate routine so that it can be used anywhere
 * waitpid or wait are called.
 */
void HandleWaitStatus( pid_t pid, int client_stat, exit_state_t sigstate,
		       int has_finalized ) 
{
    /* Get the status of the exited process */
    if (WIFEXITED(client_stat)) {
	/* true if the process exited normally */
	exitstatus[num_exited].rc = WEXITSTATUS(client_stat);
    }
    else {
	exitstatus[num_exited].rc = -1; /* For unknown, since valid
					   returns in 0-255 */
    }
	
    if (WIFSIGNALED(client_stat)) {
	exitstatus[num_exited].sig        = WTERMSIG(client_stat);
	exitstatus[num_exited].exit_state = sigstate;
	num_aborted++;
    }
    else {
	exitstatus[num_exited].sig= 0;
	exitstatus[num_exited].exit_state = 
	    has_finalized ? NORMAL : NOFINALIZE;

    }
}

/*
 * Wait on the process in fdentry[idx].  Do a blocking wait if 
 * requested.  If sigstate is not "NORMAL", set the exit state for 
 * the process to this value if it exits with a signal.  This is used
 * to separate processes that died because mpiexec sent them a signal
 * from processes that died because they received a signal from a 
 * different source (e.g., SIGFPE or SIGSEGV)
 */
int waitOnProcess( int idx, int blocking, exit_state_t sigstate )
{
    int client_stat, rc, has_finalized;
    pid_t pid;

    /* Careful here: we may want to use WNOHANG; wait a little, then
       do something like kill the process */
    if (debug) {
	DBG_FPRINTF( stderr, "Waiting on status of process %d\n",
		 fdtable[idx].pid );
	fflush( stderr );
    }
    pid = fdtable[idx].pid;
    if (pid <= 0) return -1;

    if (blocking)
	rc = waitpid( pid, &client_stat, 0 );
    else {
	rc = waitpid( pid, &client_stat, WNOHANG );
	if (rc == 0) return 0;
    }
    if (rc < 0) {
	MPIU_Internal_error_printf( "Error waiting for process!" );
	perror( "Reason: " );
	return 0;
    }
    if (debug) {
	DBG_FPRINTF( stderr, "Wait on %d completed\n", fdtable[idx].pid );
	fflush( stderr );
    }

    has_finalized = fdtable[idx].state == FINALIZED;
    HandleWaitStatus( pid, client_stat, sigstate, has_finalized );

    num_exited++;
    return 0;
}
