#include <string.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <unistd.h>
#include "util.h"

bool parentDirectory(char * path)
{
    int n = strlen(path) - 1;

    if (path[n] == '/')
        n--;

    while (n > 0 && path[n] != '/')
        n--;

    if (path[n] != '/')
        return false;

    path[ n + 1 ] = '\0';
    return true;
}

bool removeExtension(char * path)
{
    int n = strlen(path) - 1;

    while(n > 0 && path[ n ] != '.')
        n--;

    if (n == 0 || path[n - 1] == '/')
        return false;

    path[n] = '\0';

    return true;
}

char * executablePath()
{
    char * path = malloc( PATH_MAX + 1 );
    if (readlink("/proc/self/exe", path, PATH_MAX) == -1)
        return NULL;
    path[PATH_MAX] = '\0';
    path = realloc(path, strlen(path) + 1);
    return path;
}