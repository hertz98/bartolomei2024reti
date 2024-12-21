#include "sqlite_helper.h"
#include <unistd.h>

bool db_init(sqlite3 **connection, char *path)
{
    if(!sqlite3_open(path, connection))
    {
        //create tables
        return true;
    }
    else
        return false;
}

bool db_create(sqlite3 **connection, char *path)
{

    return false;
}

bool db_isPlayed(sqlite3 *connection, char *player, char *topic)
{
    return false;
}

bool db_setPlayed(sqlite3 *connection, char *player, char *topic, int score)
{
    return false;
}

void db_close(sqlite3 **connection)
{

}
