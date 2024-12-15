#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TOPIC_DIR "./data/topics/"

#include "../server/topic.h"


struct TopicsContext topics_context;

int main(int argc, char ** argv)
{
    topicsInit(&topics_context, TOPIC_DIR);

    topicsLoader(&topics_context);
    topicsFree(&topics_context);
    return 0;
}