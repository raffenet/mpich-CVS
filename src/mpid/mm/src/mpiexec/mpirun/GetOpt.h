#ifndef GETOPT_H
#define GETOPT_H

#ifdef WSOCK2_BEFORE_WINDOWS
#include <winsock2.h>
#endif
#include <windows.h>

bool GetOpt(int &argc, LPTSTR *&argv, LPTSTR flag);
bool GetOpt(int &argc, LPTSTR *&argv, LPTSTR flag, int *n);
bool GetOpt(int &argc, LPTSTR *&argv, LPTSTR flag, long *n);
bool GetOpt(int &argc, LPTSTR *&argv, LPTSTR flag, double *d);
bool GetOpt(int &argc, LPTSTR *&argv, LPTSTR flag, LPTSTR str);

#endif
