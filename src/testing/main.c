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
    topicsFree(&topics_context);
    return 0;
}