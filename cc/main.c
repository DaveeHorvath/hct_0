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
#include <wiringPi.h>
#include <softPwm.h>

#define LedPinRed    0
#define LedPinGreen  1
#define LedPinBlue   2

void ledInit(void)
{
    softPwmCreate(LedPinRed,  0, 100);
    softPwmCreate(LedPinGreen,0, 100);
    softPwmCreate(LedPinBlue, 0, 100);
}

void ledColorSet(int r, int g, int b)
{
    softPwmWrite(LedPinRed,   r);
    softPwmWrite(LedPinGreen, g);
    softPwmWrite(LedPinBlue,  b);
}

void setFdNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL);
	flags |= O_NONBLOCK;
	int res = fcntl(fd, F_SETFL, flags);
}

void changeColorRed()
{
    fprintf(stderr, "red\n");
    ledColorSet(0, 100, 100);
}

void changeColorGreen()
{
    fprintf(stderr, "green\n");
    ledColorSet(100, 100, 0);
}

int main(void)
{
    const int EPOLL_SIZE = 20;
    int polled;
    struct epoll_event temp_event;
    struct epoll_event *events;
    int trackedFds = 3;
    int eventAmount;

    if(wiringPiSetup() == -1){
        printf("setup wiringPi failed !");
        return 1;
    }

    ledInit();
    ledColorSet(0,100,100);
    changeColorRed();

    polled = epoll_create(EPOLL_SIZE);
    if (polled == -1)
    {
        printf("FAILED POLL EXITING");
        return 1;
    }
    events = (struct epoll_event *)calloc(EPOLL_SIZE * 2, sizeof(struct epoll_event));

    int ircPipe[2];
    pipe(ircPipe);
    printf("ircPipe: %d %d\n", ircPipe[0], ircPipe[1]);
    int authPipe[2];
    pipe(authPipe);
    printf("authPipe: %d %d\n", authPipe[0], authPipe[1]);
    int extPipe[2];
    pipe(extPipe);
    printf("extPipe: %d %d\n", extPipe[0], extPipe[1]);
    // start irc listener and write the output to one of the pipe-fds

    pid_t ir = fork();
    if (ir == 0)
    {
        printf("#### Started ir process with pid %d\n", ir);
        dup2(ircPipe[1], 1);
        system("bash -c 'mode2 -d /dev/lirc1'");
        exit(1);
    }
    else
    {
        // start the python project and use it at a second filedescriptor
        pid_t auth = fork();
        // pid_t auth = 1;
        if (auth == 0)
        {
            printf("#### Started auth process with pid %d\n", ir);
            dup2(authPipe[1], 1);
            system("python3 -u detector.py --ref tattoo.jpg --display");
            exit(1);
        }
        else
        {
            pid_t ext = fork();
            // pid_t ext = 1;
            if (ext == 0)
            {
                printf("#### Started webserver process with pid %d\n", ir);
                dup2(extPipe[1], 1);
                dup2(extPipe[1], 2);
                dup2(extPipe[1], 0);
                system("python3 -m http.server -d ./www/");
                exit(1);
            }
            else
            {
                setFdNonBlocking(ircPipe[0]);
                setFdNonBlocking(authPipe[0]);
                setFdNonBlocking(extPipe[0]);
                temp_event.events = EPOLLIN | EPOLLOUT;
	            temp_event.data.fd = ircPipe[0];
                if (epoll_ctl(polled, EPOLL_CTL_ADD, ircPipe[0], &temp_event) < 0)
                    printf("failed output to poll");
                temp_event.data.fd = authPipe[0];
                if (epoll_ctl(polled, EPOLL_CTL_ADD, authPipe[0], &temp_event) < 0)
                    printf("failed output to poll");
                temp_event.data.fd = extPipe[0];
                if (epoll_ctl(polled, EPOLL_CTL_ADD, extPipe[0], &temp_event) < 0)
                    printf("failed output to poll");

                // epoll the fds:
                // upon input change the state: authenticated, connected to fd
                bool authorized = true;
                size_t bufferSize = 24;
                char *buffer = (char *)calloc(bufferSize, sizeof(char));
                while (1)
                {
                    eventAmount = epoll_wait(polled, events, EPOLL_SIZE, -1);
                    printf("checked poll: %d\n", eventAmount);
                    if (eventAmount == -1)
                    {
                        perror("");
                    }
                    for (int i = 0; i < eventAmount; i++)
                    {
                        printf("%d\n", events[i].data.fd);
                        FILE *fp = fdopen(events[i].data.fd, "r"); // Convert to FILE* for getline
                        getline(&buffer, &bufferSize, fp);
                        printf("#%s#\n", buffer);
                        // ir execution
                        if (events[i].data.fd == ircPipe[0] && authorized)
                        {
                            if (strncmp(buffer, "time", 4))
                                changeColorGreen();
                            else
                                changeColorRed();

                        }
                        // opencv execution
                        if (events[i].data.fd == authPipe[0])
                        {
                            printf("balls1\n");
                            if (strstr(buffer, "off"))
                            {
                                authorized = false;
                                changeColorRed();
                                fprintf(stderr, "<>Access revoked<>\n");
                            }
                            if (strstr(buffer, " on"))
                            {
                                authorized = true;
                                fprintf(stderr, "<>Access granted<>\n");
                            }
                        }
                        // webserv execution
                        if (events[i].data.fd == extPipe[0])
                        {
                            if (strstr(buffer, "GET /red"))
                                changeColorRed();
                            if (strstr(buffer, "GET /green"))
                                changeColorGreen();
                            if (strstr(buffer, "GET /blue"))
                                ledColorSet(100, 0, 100);
                            if (strstr(buffer, "GET /white"))
                                ledColorSet(100, 100, 100);
                            if (strstr(buffer, "GET /black"))
                                ledColorSet(0, 0, 0);
                        }
                    }
                }
            }
        }
    }
}
