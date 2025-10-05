#include <stdbool.h>
#include <softPwm.h>

#define LedPinRed    0
#define LedPinGreen  1
#define LedPinBlue   2

void ledColorSet(int r, int g, int b)
{
    softPwmWrite(LedPinRed,   r);
    softPwmWrite(LedPinGreen, g);
    softPwmWrite(LedPinBlue,  b);
}

void highThreathMode()
{
    ledColorSet(0, 100, 100);
}

void safeMode()
{
    ledColorSet(100, 100, 0);
}

void execIRC(char *buffer, bool* authorized)
{
    if (strncmp(buffer, "time", 4))
        safeMode();
    else
        highThreathMode();
}

void execOpenCV(char *buffer, bool* authorized)
{
    if (strstr(buffer, "off"))
    {
        *authorized = false;
        highThreathMode();
        printf("<>Access revoked<>\n");
    }
    if (strstr(buffer, " on"))
    {
        *authorized = true;
        fprintf("<>Access granted<>\n");
    }
}

void execWebserv(char *buffer, bool* authorized)
{
    if (strstr(buffer, "GET /red"))
        highThreathMode();
    if (strstr(buffer, "GET /green"))
        safeMode();
    if (strstr(buffer, "GET /blue"))
        ledColorSet(100, 0, 100);
    if (strstr(buffer, "GET /white"))
        ledColorSet(100, 100, 100);
    if (strstr(buffer, "GET /black"))
        ledColorSet(0, 0, 0);
}