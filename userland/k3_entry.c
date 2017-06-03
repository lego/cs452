#include <basic.h>
#include <bwio.h>
#include <k2_entry.h>
#include <nameserver.h>
#include <kernel.h>
#include <ts7200.h>

void k3_entry_task() {
  bwprintf(COM2, "k3_entry\n\r");
  // First things first, make the nameserver
  Create(1, &nameserver);

  volatile int volatile *flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
  volatile int volatile *data = (int *)( UART2_BASE + UART_DATA_OFFSET );

  while (!( *flags & RXFF_MASK ));
  char x = *data;
}
