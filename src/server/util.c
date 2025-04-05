#define _DEFAULT_SOURCE
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <ctype.h>
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

int stricmp(const char *string1, const char *string2)
{
    int i = 0;
    while (string1[i] != '\0' && string2[i] != '\0')
    {
        char tmp1 = string1[i],
             tmp2 = string2[i];

        if (tmp1 >= 'A' && tmp1 <= 'Z')
            tmp1 += 'a' - 'A';

        if (tmp2 >= 'A' && tmp2 <= 'Z')
            tmp2 += 'a' - 'A';

        if (tmp1 != tmp2)
            return tmp1 - tmp2;
        
        i++;
    }

    if (string1[i] == '\0' && string2[i] == '\0')
        return 0;
    else if (string1[i] == '\0')
        return -1;
    else
        return 1;
}

bool isAlphaNumeric(const unsigned char *string)
{
    for (int i = 0; string[i]; i++)
    {
        if (!isalnum(string[i]))
            return false;
    }
    return true;
}

void shuffleArrayPtr(void **array, int arraySize)
{
    for (int i = arraySize - 1; i >= 0; i--)
    {
        int j = rand() % (arraySize - 1);
        // exchange
        void *tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    } 
}

int strcpyResize(char **dst, const char *src, int *allocatedSize, int pos)
{
    if (!dst || !src || !allocatedSize || pos < 0) 
        return -1;
    
    int len = 0; // Used for both indexing and measuring size
    
    while (src[len] != '\0')
    {
        if (pos + len + 1 >= *allocatedSize) // If the position + current lenght + null exceed the size
        {
            while (pos + len + 1 >= *allocatedSize)
                *allocatedSize *= 2;  // Exponential growth

            char *tmp = (char *) realloc(*dst, *allocatedSize);
            if (!tmp)
                return -1;
            
            *dst = tmp;
        }

        (*dst)[pos + len] = src[len];
        
        len++;
    }

    (*dst)[pos + len] = '\0';  // Terminating character

    return len;  // Return the copied size (without the terminating character)
}

bool removeNumbering(char *string)
{
    int i = 0;
    while(string[i] != '\0')
    {
        if ((string[i] >= 'A' && string[i] <= 'Z') ||
            (string[i] >= 'a' && string[i] <= 'z'))
        {
            if( i == 0 )
                return false;
            memmove(string, string + i, strlen(string) - i + 1);
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

// This algotithm is not suitable for checking the answer because
// can be exploited easily, in fact by submitting more words I can
// try differents answer
bool wordInString(const char * string, const char * substring, int tol)
{
    int correct = 0; // Indicizza la risposta giusta
    int errors = 0; // Numero di errori

    for ( ; *string != '\0'; string++)
    {
        char tmpSub = substring[correct],
             tmpString = *string;

        if ( tmpSub == '\0' && !isalpha(tmpString))    // Se la risposta giusta termina insieme alla parola
            return true;

        if (tmpString == ' ' && tmpSub != ' ') // Inizia una nuova parola, reinizia da capo
        {
            errors = correct = 0;
            continue;
        }

        if (errors == -1) // Scarto la parola corrente
            continue;

        // Conversione da maiuscolo a minuscolo

        if (tmpSub >= 'A' && tmpSub <= 'Z')
            tmpSub += 'a' - 'A';

        if (tmpString >= 'A' && tmpString <= 'Z')
            tmpString += 'a' - 'A';

        // Test corrispondenza

        if (tmpSub != tmpString && ++errors > tol)
            errors = -1;
        else
            correct++;
    }

    // Caso in cui entrambe le stringhe terminano
    if (*string == '\0' && substring[correct] == '\0')
        return true;

    return false;
}

int stricmpTol(const char *string1, const char *string2, int tol, int small)
{
    int errors = 0;

    int i = 0;
    while (string1[i] != '\0' && string2[i] != '\0')
    {
        char tmp1 = string1[i],
             tmp2 = string2[i];

        if (tmp1 >= 'A' && tmp1 <= 'Z')
            tmp1 += 'a' - 'A';

        if (tmp2 >= 'A' && tmp2 <= 'Z')
            tmp2 += 'a' - 'A';

        // La tolleranza agli errori non si applica ai numeri
        if ((isdigit(tmp1) || isdigit(tmp2)) && (tmp1 != tmp2) )
            return tmp1 - tmp2;
        else if (tmp1 != tmp2 && ++errors > tol)
            return tmp1 - tmp2;
        
        i++;
    }

    if ( !(i <= small && errors) &&      // Non applico la tolleranza agli errori alle risposte brevi
        (string1[i] == '\0' && string2[i] == '\0') )
        return 0;
    else if (string1[i] == '\0')
        return -1;
    else
        return 1;
}