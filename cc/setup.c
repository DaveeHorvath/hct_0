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

// enduser specific setup code
int setup()
{
    if(wiringPiSetup() == -1){
        printf("setup wiringPi failed !");
        return 1;
    }
    ledInit();
}