#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DATA_DIR "./data/"

#include "../server/topic.h"


struct TopicsContext topics_context;

int main(int argc, char ** argv)
{
    topicsInit(&topics_context, DATA_DIR);

    topicsLoader(&topics_context);

    bool * played = topicsUnplayed(&topics_context, "gabriele");

    for (int i = 0; i < topics_context.nTopics; i++)
        printf("%d", played[i]);
    printf("\n");

    topicsFree(&topics_context);
    return 0;
}