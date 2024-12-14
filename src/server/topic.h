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

struct TopicsContext
{
    char *directory;
    int nTopics;
    struct Topic **topics;
};


bool topicsLoader(struct TopicsContext *);
bool topicLoad(FILE *, struct Topic *);
void topicsFree(struct Question **);
