#ifndef ENV_H_INCLUDED
#define ENV_H_INCLUDED

/* Element of the list of environment variables */
typedef struct EnvData {
    const char *name, *value;
    struct EnvData *nextData;
} EnvData;

typedef struct EnvInfo {
    int includeAll;      /* true if all environment variables should be 
			    included, false if none (other than those
			    explicitly listed below) should be included */
    EnvData *envPairs;   /* List of name,value pairs to be included */
    EnvData *envNames;   /* List of names to be included, using the 
			    current value in the environment */
} EnvInfo;

#endif
