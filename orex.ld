ENTRY (main)

program_start := 0x00218000

MEMORY
{
/* TODO: can we maybe put a memory limit that matches where we put tasks to assert no collision */
ram : ORIGIN = program_start, LENGTH = 32M - program_start
rom : ORIGIN = 0x60000000, LENGTH = 8M
}

SECTIONS
{
.data : /* Initialized data. */
{
_DataStart = . ;
*(.data)
*(.data.rel.ro)
_DataEnd = . ;
} >ram

.bss : /* Uninitialized data. */
{
_BssStart = . ;
*(.bss)
_BssEnd = . ;
} >ram


text : /* The actual instructions. */
{
*(.text)
*(.got)
*(.got.plt)
*(.rodata)
*(.glue_7)
*(.glue_7t)
} >ram

rodata : { *(.rodata.str1.4) } >ram
}
