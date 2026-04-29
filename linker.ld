/* STM32F103RC 链接脚本 - 最小化版本 */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 48K
}

/* 栈顶 = RAM 顶部，栈向下生长 */
_estack = ORIGIN(RAM) + LENGTH(RAM);

ENTRY(Reset_Handler)

SECTIONS
{
    /* 中断向量表 + 代码 */
    .text :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* 已初始化数据（Flash中存储，启动时拷贝到RAM） */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    _sidata = LOADADDR(.data);

    /* 未初始化数据（启动时清零） */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* 不清零段 */
    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        *(.noinit)
        . = ALIGN(4);
    } > RAM

    /* 堆（从 .noinit 之后到栈底） */
    .heap (NOLOAD) :
    {
        . = ALIGN(4);
        _sheap = .;
        . = . + 0x400;   /* 1KB 堆 */
        . = ALIGN(4);
        _eheap = .;
    } > RAM

    /* 栈（在堆之后，向下生长到 _estack） */
    _sstack = _eheap;
}
