#ifndef __CLOCK_SERVER_H__
#define __CLOCK_SERVER_H__

void clock_server();

/* Clock server calls */
int Delay( int tid, unsigned int delay );
int Time( int tid );
int DelayUntil( int tid, unsigned long int until );

#endif
