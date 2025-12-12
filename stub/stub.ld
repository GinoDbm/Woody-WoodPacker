/* Linker script for stub - places .data followed by .text */
SECTIONS
{
    . = 0;
    .data : { *(.data) }
    .text : { *(.text) }
}
