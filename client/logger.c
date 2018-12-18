#include "logger.h"

static bool logger_worked;
static mqd_t entry_pointer;

void initSignalCatch(sigset_t *empty, sigset_t *block);
void handlerForLogger(int signum);
void disposeLogger();

int InitLogger()
{
    logger_worked = true;
    mqd_t message_queue;
    ssize_t bytes_read;
    struct mq_attr attr;
    char buffer[MAX_MSG_SIZE + 1];
    attr.mq_flags = 0;
    attr.mq_curmsgs = 0;
    attr.mq_maxmsg = MAX_COUNT_MSG;
    attr.mq_msgsize = MAX_MSG_SIZE;

    message_queue = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY | O_NONBLOCK, 0644, &attr);
    if (message_queue == -1) 
    {
        printf("Fail to init 'message queue'\n");
        pthread_exit(-1);
    }

    entry_pointer = mq_open(QUEUE_NAME, O_WRONLY| O_NONBLOCK);
    if (entry_pointer == -1) 
    {
        printf("Fail to init entry pointer for message queue\n");
        pthread_exit(-1);
    }

    sigset_t empty, block;
    initSignalCatch(&empty, &block);

    int ready = 0;

    fd_set readers, readers_temp;
    FD_ZERO(&readers);
    FD_ZERO(&readers_temp);

    FD_SET(message_queue, &readers);

    //  log file 
    FILE *log;
    int err;
    while (logger_worked) 
    {
        readers_temp = readers;
        ready = pselect(2, &readers_temp, NULL, NULL, NULL, &empty);

        printf("receive\n");
        if (ready == 0) {
            continue;
        }

        bool sprint = true;
        bool opened = false;
        while(sprint) 
        {
            memset(buffer, 0, sizeof(buffer));
            bytes_read = mq_receive(message_queue, buffer, MAX_MSG_SIZE, NULL);
            if (bytes_read == -1 && errno == EAGAIN) 
            {
                sprint = false;
                continue;
            }

            if (!opened) {
                log = fopen(LOG_FILE, "a");
                if (log == NULL) 
                {
                    printf("lost one of log line\n");
                    continue;
                }
                opened = true;
            }

            time_t current_time = time(NULL);
            char * time_of_string = ctime(current_time);
            time_of_string[strlen(time_of_string) - 1] = '\0';
            fprintf(log, "[%s] ERROR: %s\n", time_of_string, buffer);
            
        }
        fclose(log);      
    }

    disposeLogger(&message_queue);
    pthread_exit(0);
    return;
}

void Error(char *message)
{
    int len = strlen(message);
    if (len > MAX_MSG_SIZE) 
        return;

    printf("Send\n");
    mq_send(entry_pointer, message, MAX_MSG_SIZE, 0);
    return;
}

void disposeLogger(mqd_t *queue)
{

    //  entry_pointer
    mq_close(entry_pointer);
    //  logger
    mq_close((*queue));
    mq_unlink(QUEUE_NAME);
    return;
}

void initSignalCatch(sigset_t *empty, sigset_t *block)
{
    sigemptyset(empty);
    sigemptyset(block);

    sigaddset(block, SIGUSR1);
    sigaddset(block, SIGINT);
    pthread_sigmask(SIG_BLOCK, block, empty);
    struct sigaction signals;
    signals.sa_handler = handlerForLogger;
    signals.sa_flags = 0;
    sigemptyset(&signals.sa_mask);
    sigaction(SIGUSR1, &signals, NULL);
    return;
}

void handlerForLogger(int signum)
{
    if (signum == SIGUSR1)
    {
        logger_worked = false;
    }
}