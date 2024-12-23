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

bool newlineReplace(char * path)
{
    int len = strlen(path);

    if (len >= 2 && path[len - 2] == '\r') // Caso codifica Windows
    {
        path[len - 2] = '\0';
        return true;
    }

    else if (len >= 1 && path[len - 1] == '\n') // Rimuovo il carattere di nuova linea
    {
        path[len - 1] = '\0';
        return true;
    }
    
    return false;
}

bool removeNumbering(char *string)
{
    int i = 0;
    while(string[i] != '\0')
    {
        if ( (string[i] >= 'A' && string[i] <= 'Z') ||
         (string[i] >= 'a' && string[i] <= 'z'))
        {
            strcpy(string, string + i);
            return true;
        }
        i++;
    }
    return false;
}

char * executablePath(char * string)
{
    if (string)
    {
        if (readlink("/proc/self/exe", string, PATH_MAX) == -1)
            return NULL;
        return string;
    }
    else
    {
        char * path = malloc( PATH_MAX + 1 );
        if (readlink("/proc/self/exe", path, PATH_MAX) == -1)
            return NULL;
        path[PATH_MAX] = '\0';
        path = realloc(path, strlen(path) + 1);
        return path;
    }
}

