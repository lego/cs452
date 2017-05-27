ENTRY (main)

MEMORY
{
  ram : ORIGIN = 0x00218000, LENGTH = 32M - 0x00218000
  rom : ORIGIN = 0x60000000, LENGTH = 8M
}

SECTIONS
{
  /* Initialized data. */
  .data : {
    _DataStart = . ;
    *(.data)
    *(.data.rel.ro)
    _DataEnd = . ;
  } >ram

  /* Uninitialized data. */
  .bss : {
    _BssStart = . ;
    *(.bss)
    _BssEnd = . ;
  } >ram

  /* The actual instructions. */
  text : {
    *(.text)
    *(.got)
    *(.got.plt)
    *(.rodata)
    *(.glue_7)
    *(.glue_7t)
  } >ram

  rodata : {
    *(.rodata.str1.4)
  } >ram

  /* Begin the task stack */
  _TaskStackSize = 0x10000 ;
  _TaskStackStart = ALIGN(0x1000) + _TaskStackSize ;
  /* Temp this is what is used */
  . = 0x250000 ;
  /* Ensure there is enough space for 100 tasks */
  . += _TaskStackSize * 100 ;
}
