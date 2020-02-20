# Operating Systems

## Project 0
To create the new test program alarm-mega I created the alarm-mega.ck file in /src/tests/threads/ which is based off of the code in alarm-multiple.ck.
The only difference between the two file is that check_alarm is set to 70 in alarm-mega intead of 7. Then to figure out which files used alarm_multiple
I used grep -r alarm-multiple *. In every file where alarm_multiple was I added in alarm-mega. In /src/tests/threads/tests.h I added in a new test function prototype called
test_alarm_mega. In /src/tests/threads/alarm-wait.c, I added in a new function called test_alarm_mega which is very similar to test_alarm_multiple, but it uses 5,70 instead 
of 5,7 when calling test_sleep. In /src/tests/threads/tests.c I added in {"alarm-mega", test_alarm_mega} which looks like it will run the test_alarm_mega function when 
the alarm-mega keyword is given.

## Project 1
In this project we created the timer for pintos which allows threads to sleep at specfic intervale. The code is located in /src/devices/timer.c and /src/devices/timer.h. We also worked on priority scheduling which worked out for the most part, but did not pass all tests so we reverted back to Project 0 so we can continue working on Project 2. We also kept the implementation of timer becuase it will be used in later projects.
