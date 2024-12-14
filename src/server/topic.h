#include <stdbool.h>

struct Question 
{
    char * question;
    char * answer;
};

struct Topic 
{
    char * name;
    struct Question * questions;
};

#ifndef PATH_BUFFER_SIZE
#define PATH_BUFFER_SIZE 4096
#endif

struct TopicsContext
{
    char directory[PATH_BUFFER_SIZE];
    int nTopics;
    struct Topic **topics;
};

bool topicsInit(struct TopicsContext *, char *);
bool topicsLoader(struct TopicsContext *);
bool topicLoad(FILE *, struct Topic *);
void topicsFree(struct Question **);
