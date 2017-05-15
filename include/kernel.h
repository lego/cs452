#ifndef __KERNEL_H__
#define __KERNEL_H__

// TODO: maybe make these inline?

int Create( int priority, void (*code)( ) );
int MyTid( );
int MyParentTid( );
void Pass( );
void Exit( );

#endif
