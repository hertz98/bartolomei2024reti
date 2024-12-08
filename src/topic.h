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


bool topicsLoader(struct Question **);
bool topicLoad(char *, struct Topic *);
void topicsFree(struct Question **);
