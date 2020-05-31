#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <hiredis.h>

#define NUM_THREADS 4
#define COUNT 11

struct func_args {
    int leader;
    redisContext *c;
    char *name;
};

static pthread_mutex_t cnt_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lst_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Once the threads block on the blocking pop command,
 * you can start them again by first setting CNT to 0,
 * then in an exec command you can insert "GO" into each
 * thread's queue to make them start again. You can do this
 * with the redis-cli and you can submit multiple commands
 * at one time using MULTI and EXEC commands. If you want to
 * stop a thread, put STOP instead of GO in its signal queue.
 * to stop the program, stop each thread then the program will
 * exit.
 *
 * */

void *sprint(void *arg) {
    int go = 1;
    char tag[32];
    struct func_args *args = (struct func_args *) arg;
    int cnt = 0;
    redisReply *reply;
    const char *blank = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    while(go) {
        // blank out the tag
        sprintf(tag, "%s", blank);
        sprintf((tag + 16), "%s", blank);
        //get the count
        pthread_mutex_lock(&cnt_mutex);
        reply = redisCommand(args->c,"INCR cnt");
        cnt = (int)(reply->integer);
        freeReplyObject(reply);
        // make sure to lock the list before letting go of the count
        pthread_mutex_lock(&lst_mutex);
        pthread_mutex_unlock(&cnt_mutex);
        if (cnt < COUNT) {
            // go ahead and add the current count to the list
            sprintf(tag, "%s%s%i%c", args->name, " : ", cnt, '\0');
            reply = redisCommand(args->c, "LPUSH mylist %s", tag);
            pthread_mutex_unlock(&lst_mutex);
            freeReplyObject(reply);
            printf("%s%s%d\n", args->name, " got : ", cnt);
        } else {
            // the count is to high, wait for further signal
            pthread_mutex_unlock(&lst_mutex);
            printf("%s%s", args->name, " BLOCKED\n");
            reply = *(((
                    (redisReply *)redisCommand(args->c, "BRPOP %s%s 0", args->name, "->signal")
                    )->element) + 1);
            if (*(reply->str) == 'S')
                go = 0;
            else
                printf("%s%s", args->name, " ZOMMIN\n");
        }
    }
    printf("%s%s", args->name, " is DONE !!!\n");
    return arg;
}

int main(int argc, char **argv) {
    printf("Start program ...\n");
    pthread_t threads[NUM_THREADS];
    redisContext *conns[NUM_THREADS];
    char names[NUM_THREADS][16];
    struct func_args thread_args[NUM_THREADS];
    const char *hostname = "127.0.0.1";
    int port = 6379;
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    for(int i = 0; i < NUM_THREADS; i++) {
        conns[i] = redisConnect(hostname, port);
        if (conns[i] == NULL || conns[i]->err) {
            if (conns[i]) {
                printf("Connection error: %s\n", conns[i]->errstr);
                redisFree(conns[i]);
            } else {
                printf("Connection error: can't allocate redis context\n");
            }
            exit(1);
        }
    }

    // initialize Redis data objects
    freeReplyObject(redisCommand(conns[0],"SET cnt 0"));
    freeReplyObject(redisCommand(conns[0],"DEL mylist"));

    // Initialize threads
    for(int i = 0; i < NUM_THREADS; i++) {
        strcpy(names[i], "thread->");
        sprintf(&names[i][8], "%c%c", ('A' + i), '\0');
        thread_args[i].name = names[i];
        thread_args[i].c = conns[i];
        thread_args[i].leader = (i != 0);
        pthread_create(&threads[i], NULL, &sprint, &thread_args[i]);
    }

    // clean up threads
    void *ret;
    for(int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], &ret);

    /* Disconnects and frees the context */
    for(int i = 0; i < NUM_THREADS; i++)
        redisFree(conns[i]);
    // clean up mutexes
    pthread_mutex_destroy(&cnt_mutex);
    pthread_mutex_destroy(&lst_mutex);

    return 0;
}