#ifndef __CLOCK_SERVER_H__
#define __CLOCK_SERVER_H__

void clock_server();

/* Clock server calls */
int Delay(unsigned int delay );
int Time();
int DelayUntil(unsigned long int until );

#endif
