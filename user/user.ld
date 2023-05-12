OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(main)

SECTIONS
{
	. = 0x0;

	.text : {
		*(.text .text.*)
	}

  . = ALIGN(4K);

	.rodata : {
		*(.rodata .rodata.*)
	}

  . = ALIGN(4K);

	.data : {
		*(.data .data.*)
	}

	.bss : {
		*(.bss .bss.*)
	}
}
