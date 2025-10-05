# hct_0
detection code from: https://github.com/Deepanker5/junction/blob/main/detector.py 
## usage:
this solution is tested only on our system, as an external IR receiver has to be connected to the raspberry pi as well. a quick demo of it can be seen at the end of our video as well and we would be happy to provide the assembled system for review.
## design
the main pipe and child process management is done in C in a way that doesnt rely on any modern features, so any device capable of compiling C98 code should be able to run it. We wanted to separate the commands providing the inputfeeds, these happen over the 3 pipes. To see a cleaner solution, please check out the refactor branch!
## requirements
gpio expansion board, lirc driver and device configuration. The directory structure also has to change. There is a bigger set of requirements for the python packages for the pattern recognition, these will be added to the other repo.
