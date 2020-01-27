CIS 560 Project 0
By Nickalas Porsch

To create the new test program alarm-mega I created the alarm-mega.ck file in /src/tests/threads/ which is based off of the code in alarm-multiple.ck.
The only difference between the two file is that check_alarm is set to 70 in alarm-mega intead of 7. Then to figure out which files used alarm_multiple
I used grep -r alarm-multiple *. In every file where alarm_multiple was I added in alarm-mega. In /src/tests/threads/tests.h I added in a new test function prototype called
test_alarm_mega. In /src/tests/threads/alarm-wait.c, I added in a new function called test_alarm_mega which is very similar to test_alarm_multiple, but it uses 5,70 instead 
of 5,7 when calling test_sleep. In /src/tests/threads/tests.c I added in {"alarm-mega", test_alarm_mega} which looks like it will run the test_alarm_mega function when 
the alarm-mega keyword is given.

