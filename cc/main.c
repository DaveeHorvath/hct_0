#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <stdbool.h>
#include <regex.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

void execIRC(char *buffer, bool* authorized);
void execOpenCV(char *buffer, bool* authorized);
void execWebserv(char *buffer, bool* authorized);
int setup();
void highThreathMode();
void safeMode();

void setFdNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    flags |= O_NONBLOCK;
    int res = fcntl(fd, F_SETFL, flags);
}

typedef struct listener
{
    char *name;
    int pipes[2];
    pid_t process;
    const char *command;
    bool requiresAuth;
    void (*exec)(char *, bool *);
} t_listener;

int main(void)
{
    // epoll info
    const int EPOLL_SIZE = 20;
    int polled;
    struct epoll_event temp_event;
    struct epoll_event *events;
    int trackedFds = 3;
    int eventAmount;

    // setup listening devices
    setup();
    // default value
    highThreathMode();

    polled = epoll_create(EPOLL_SIZE);
    if (polled == -1)
    {
        printf("FAILED POLL EXITING");
        return 1;
    }
    events = (struct epoll_event *)calloc(EPOLL_SIZE * 2, sizeof(struct epoll_event));

    // commands setup
    t_listener commands[3] = {
        {.exec = &execIRC, .name = "IR receiver", .command = "bash -c 'mode2 -d /dev/lirc1'", .requiresAuth = true},
        {.exec = &execOpenCV, .name = "Auth detector", .command = "python3 -u detector.py --ref tattoo.jpg --display", .requiresAuth = false},
        {.exec = &execWebserv, .name = "Remote controls", .command = "python3 -m http.server -d ./www/'", .requiresAuth = false},
    };

    for (size_t i = 0; i < 3; i++)
    {
        if (pipe(commands[i].pipes) != -1)
            printf("%s pipe opened: %d %d\n", commands[i].name, commands[i].pipes[0], commands[i].pipes[1]);
        pid_t id = fork();
        if (id == 0)
        {
            dup2(commands[i].pipes[1], 1);
            dup2(commands[i].pipes[1], 2);
            system(commands[i].command);
            exit(1);
        }
        else
        {
            printf("### Starting %s with pid %d", commands[i].name, id);
            commands[i].process = id;
            setFdNonBlocking(commands[i].pipes[0]);
            temp_event.events = EPOLLIN | EPOLLOUT;
            temp_event.data.fd = commands[i].pipes[0];
            if (epoll_ctl(polled, EPOLL_CTL_ADD, commands[i].pipes[0], &temp_event) < 0)
                printf("Failed to add %s to epoll", commands[i].name);
        }
    }

    // epoll the fds:
    // upon input change the state: authenticated, connected to fd
    bool authorized = true;
    size_t bufferSize = 24;
    char *buffer = (char *)calloc(bufferSize, sizeof(char));
    while (1)
    {
        eventAmount = epoll_wait(polled, events, EPOLL_SIZE, -1);
        if (eventAmount == -1)
        {
            perror("");
        }
        for (int i = 0; i < eventAmount; i++)
        {
            FILE *fp = fdopen(events[i].data.fd, "r"); // Convert to FILE* for getline
            getline(&buffer, &bufferSize, fp);
            for (int cm = 0; cm < 3; cm++)
            {
                if (commands[cm].pipes[0] == events[i].data.fd && (!commands[cm].requiresAuth || authorized))
                {
                    (*commands[cm].exec)(buffer, &authorized);
                }
            }
        }
    }
}