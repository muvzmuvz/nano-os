ENTRY(multiboot_entry)

SECTIONS
{
    . = 1M;

    .multiboot : { *(.multiboot) }
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) *(COMMON) }

    end_of_kernel = .; /* <<< Добавим этот символ */
}