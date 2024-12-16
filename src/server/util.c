#include <string.h>
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