#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <stdbool.h>
#include <regex.h>
#include <string.h>

#define COLORRED 100
#define COLORGREEN 200

void changeColorRed()
{
    printf("red\n");
}

void changeColorGreen()
{
    printf("green\n");
}

int main(void)
{
    const int EPOLL_SIZE = 20;
    int polled;
    struct epoll_event temp_event;
    struct epoll_event *events;
    int trackedFds = 3;
    int eventAmount;

    polled = epoll_create(EPOLL_SIZE);
    if (polled == -1)
    {
        printf("FAILED POLL EXITING");
        return 1;
    }
    events = (struct epoll_event *)calloc(EPOLL_SIZE * 2, sizeof(struct epoll_event));

    int ircPipe[2];
    pipe(ircPipe);
    int authPipe[2];
    pipe(authPipe);
    int extPipe[2];
    pipe(extPipe);
    // start irc listener and write the output to one of the pipe-fds

    pid_t ir = fork();
    if (ir == 0)
    {
        printf("#### Started ir process with pid %d\n", ir);
        dup2(1, ircPipe[0]);
        system("bash -c 'mode2 -d /dev/lirc1 | grep pulse'");
    }
    else
    {
        // start the python project and use it at a second filedescriptor
        pid_t auth = fork();
        if (auth == 0)
        {
            printf("#### Started auth process with pid %d\n", ir);
            dup2(1, authPipe[0]);
            system("python3 <evil-opencv-model-placeholder>");
        }
        else
        {
            pid_t ext = fork();
            if (ext == 0)
            {
                printf("#### Started external process with pid %d\n", ir);
                dup2(1, extPipe[0]);
                system("# some random webserver");
            }
            else
            {
                if (epoll_ctl(polled, EPOLL_CTL_ADD, ircPipe[1], &temp_event) < 0)
                    printf("Added irc output to poll");
                if (epoll_ctl(polled, EPOLL_CTL_ADD, authPipe[1], &temp_event) < 0)
                    printf("Added irc output to poll");
                if (epoll_ctl(polled, EPOLL_CTL_ADD, extPipe[1], &temp_event) < 0)
                    printf("Added irc output to poll");

                // epoll the fds:
                // upon input change the state: authenticated, connected to fd
                // ssize_t getline(char **restrict lineptr, size_t *restrict n,
                //    FILE *restrict stream);
                bool authorized = false;
                char *buffer = (char *)calloc(512, sizeof(char));
                size_t bufferSize = 512;

                while (1)
                {
                    eventAmount = epoll_wait(polled, events, EPOLL_SIZE, 0);
                    if (eventAmount == -1)
                    {
                        perror("");
                    }
                    for (int i = 0; i < eventAmount; i++)
                    {
                        // ir execution
                        if (events[i].data.fd == ircPipe[1] && authorized)
                        {
                            fgets(buffer, bufferSize, &ircPipe[1]);
                            int code = 0;
                            for (int c = 0; buffer[c]; c++)
                            {
                                if (isdigit(buffer[c]))
                                {
                                    code = atoi(&buffer[c]);
                                }
                            }
                            switch (code)
                            {
                            case COLORRED:
                                changeColorRed();
                                break;
                            case COLORGREEN:
                                changeColorGreen();
                                break;

                            default:
                                break;
                            }
                        }
                        // opencv execution
                        if (events[i].data.fd == authPipe[1])
                        {
                            fgets(buffer, bufferSize, &authPipe[1]);
                            if (strncmp(buffer, "true", 4))
                                authorized = true;
                            if (strncmp(buffer, "false", 5))
                                authorized = false;
                        }
                        // webserv execution
                        if (events[i].data.fd == extPipe[1])
                        {
                            fgets(buffer, bufferSize, &extPipe[1]);
                            if (strncmp(buffer, "red", 3))
                                changeColorRed();
                            if (strncmp(buffer, "green", 5))
                                changeColorGreen();
                        }
                    }
                }
            }
        }
    }
}
