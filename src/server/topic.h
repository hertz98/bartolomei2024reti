#include <stdbool.h>
#include "list.h"

#ifndef PATH_BUFFER_SIZE
#define PATH_BUFFER_SIZE 4096
#endif

#ifndef FILE_NAME_SIZE
#define FILE_NAME_SIZE 256
#endif

typedef struct Question 
{
    char * question;
    char * answer;
} Question;

typedef struct Topic 
{
    char name[FILE_NAME_SIZE];
    Node * questions;
} Topic;

typedef struct TopicsContext
{
    char directory[PATH_BUFFER_SIZE];
    int nTopics;
    struct Topic * topics;
} TopicsContext;

bool topicsInit(struct TopicsContext *, char *);
bool topicsLoader(struct TopicsContext *);
bool topicLoad(FILE *, struct Topic *);
void topicsFree(struct Question **);
