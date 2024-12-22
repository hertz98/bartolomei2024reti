#include <stdbool.h>

/// @brief Returns the parent directory of a path
/// @param  path The string containing the path
/// @return true if succeded
bool parentDirectory(char * path);

/// @brief Returns the path, with the filename and not the extension
/// @param path The string containing the path
/// @return true if succeded
bool removeExtension(char * path);

/// @brief Returns the location of the executable file
/// @param string String where to write if NULL it will be allocated dynamically
/// @return The string containing the executable path
char * executablePath(char * string);

/// @brief Replace the newline character '\\n' with endline character '\\0'
/// @param path String containing '\n' character
void newlineReplace(char * path);