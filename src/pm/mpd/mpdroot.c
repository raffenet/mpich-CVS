/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpdroot.h"

int main(int argc, char *argv[])
{
    int i, rc, sock;
    char env_unix_socket[NAME_LEN];
    struct sockaddr_un sa;
    char console_name[NAME_LEN], cmd[NAME_LEN];
    struct passwd *pwent;

    if ((pwent = getpwuid(getuid())) == NULL)    /* for real id */
    {
	printf("%s: getpwnam failed",argv[0]);
	exit(-1);
    }

    /* if just want help, go straight to the program; else setup console */
    if (argc == 2  &&  strncmp("--help", argv[1], 6) == 0)
    {
	/* do not need console */
    }
    else
    {
        /* setup default console */
        snprintf(console_name,NAME_LEN,"/tmp/mpd2.console_%s", pwent->pw_name );

        /* handle undocumented options: */
	/*   'use user console' even if setuid root; */
	/*   either set an env var or place --mpduuc on cmd-line */
        if (getenv("MPD_USE_USER_CONSOLE"))
        {
	    /* nothing to do; just stick with default console */
        }
        else if (argc > 1  &&  strncmp(argv[1],"--mpduuc",4) == 0)
        {
            for (i=1; i < argc; i++)
                argv[i] = argv[i+1];
            argc--;
        }
        else if (geteuid() == 0)    /* if I am running as setuid root */
        {
            snprintf(console_name,NAME_LEN,"/tmp/mpd2.console_root");
        }
    
        bzero( (void *)&sa, sizeof( sa ) );
        sa.sun_family = AF_UNIX;
        strncpy(sa.sun_path,console_name, sizeof(sa.sun_path)-1 );
        sock = socket(AF_UNIX,SOCK_STREAM,0);
        if (sock < 0)
        {
            printf("failed to get socket for %s\n", console_name);
            exit(-1);
        }
        rc = connect(sock,(struct sockaddr *)&sa,sizeof(sa));
        if (rc < 0)
        {
            /*****
            printf("cannot connect to local mpd at: %s\n", console_name);
	    printf("probable cause:  no mpd daemon on this machine\n");
	    printf("possible cause:  unix socket %s has been removed\n", console_name);
            exit(-1);
            *****/
        }
        else
        {
            snprintf(env_unix_socket,NAME_LEN,"UNIX_SOCKET=%d",sock);
            putenv(env_unix_socket);
        }
    }

    setreuid(getuid(),getuid());
    setregid(getgid(),getgid());

    strncpy(cmd,argv[0],NAME_LEN);
    strncat(cmd,".py",NAME_LEN);
    argv[0] = cmd;
    execvp(cmd,&argv[0]);
    printf("mpdroot failed to exec :%s:\n",cmd);

    return(0);
}
