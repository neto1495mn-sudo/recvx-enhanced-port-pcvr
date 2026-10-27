OUTPUT_FORMAT("elf32-sh", "elf32-sh", "elf32-sh")
OUTPUT_ARCH(sh)
ENTRY(start) /* start */
/* SEARCH_DIR(/usr/src/dossh4elf/sh-hms-elf/lib);*/
/* Do we need any of these for elf?
   __DYNAMIC = 0;    */
SECTIONS
{
  . = 0x8c008000;
  _IP = .;
  _IP_start = . + 0x300;
  ip : { *(IP) }

  . = 0x8c010000;
  __START_DSGLH = .;
  DSGLH : { *(DSGLH) }
  __END_DSGLH = .;

  . = ALIGN(4);
  __START_DSGLE = .;
  DSGLE : { *(DSGLE) }
  __END_DSGLE = .;

  . = ALIGN(4);
  __START_P = .;
  P : {	*(P) }
  __END_P = .;

  . = ALIGN(4);
  __START_C = .;
  C : { *(C) }
  __END_C = .;

  . = ALIGN(32);
  __START_C32 = .;
  C32 : { *(C32) }
  __END_C32 = .;

  . = ALIGN(4);
  __START_D = .;
  D : { *(D) }
  __END_D = .;

  . = ALIGN(32);
  __START_D32 = .;
  D32 : { *(D32) }
  __END_D32 = .;

  . = ALIGN(4);
  __START_R = .;
  R : { *(R) }
  __END_R = .;

  . = ALIGN(32);
  __START_R32 = .;
  R32 : { *(R32) }
  __END_R32 = .;

  . = ALIGN(4);
  __START_PSG = .;
  PSG : { *(PSG) }
  __END_PSG = .;

  . = ALIGN(4);
  __START_CSG = .;
  CSG : { *(CSG) }
  __END_CSG = .;

  . = ALIGN(32);
  __START_CSG32 = .;
  CSG32 : { *(CSG32) }
  __END_CSG32 = .;

  . = ALIGN(4);
  __START_DSG = .;
  DSG : { *(DSG) }
  __END_DSG = .;

  . = ALIGN(32);
  __START_DSG32 = .;
  DSG32 : { *(DSG32) }
  __END_DSG32 = .;

  . = ALIGN(4);
  __START_RSG = .;
  RSG : { *(RSG) }
  __END_RSG = .;

  . = ALIGN(32);
  __START_RSG32 = .;
  RSG32 : { *(RSG32) }
  __END_RSG32 = .;

  . = ALIGN(4);
  __START_B = .;
  B : { *(B) }
  __END_B = .;

  . = ALIGN(32);
  __START_B32 = .;
  B32 : { *(B32) }
  __END_B32 = .;

  . = ALIGN(32);
  __START_PSGSFD00 = .;
  PSGSFD00 : { *(PSGSFD00) }
  . = ALIGN(32) + 0x14;         /* 20 bytes added to replace deadstripped code. */
  __END_PSGSFD00 = .;

  . = ALIGN(4);
  __START_PSGSFD01 = .;
  PSGSFD01 : { *(PSGSFD01) }
  __END_PSGSFD01 = .;

  . = ALIGN(4);
  __START_PSGSFD02 = .;
  PSGSFD02 : { *(PSGSFD02) }
  __END_PSGSFD02 = .;

  . = ALIGN(4);
  __START_PSGSFD03 = .;
  PSGSFD03 : { *(PSGSFD03) }
  __END_PSGSFD03 = .;

  __SIZE_PSGSFD00 = __END_PSGSFD00 - __START_PSGSFD00;
  __SIZE_PSGSFD01 = __END_PSGSFD01 - __START_PSGSFD01;
  __SIZE_PSGSFD02 = __END_PSGSFD02 - __START_PSGSFD02;
  __SIZE_PSGSFD03 = __END_PSGSFD03 - __START_PSGSFD03;

  /* Read-only sections, merged into text segment: */
  /*. = 0x8c010000;*/
  .interp     : { *(.interp) 	}
  .hash          : { *(.hash)		}
  .dynsym        : { *(.dynsym)		}
  .dynstr        : { *(.dynstr)		}
  .gnu.version   : { *(.gnu.version)	}
  .gnu.version_d   : { *(.gnu.version_d)	}
  .gnu.version_r   : { *(.gnu.version_r)	}
  .rel.text      :
    { *(.rel.text) *(.rel.gnu.linkonce.t*) }
  .rela.text     :
    { *(.rela.text) *(.rela.gnu.linkonce.t*) }
  .rel.data      :
    { *(.rel.data) *(.rel.gnu.linkonce.d*) }
  .rela.data     :
    { *(.rela.data) *(.rela.gnu.linkonce.d*) }
  .rel.rodata    :
    { *(.rel.rodata) *(.rel.gnu.linkonce.r*) }
  .rela.rodata   :
    { *(.rela.rodata) *(.rela.gnu.linkonce.r*) }
  .rel.got       : { *(.rel.got)		}
  .rela.got      : { *(.rela.got)		}
  .rel.ctors     : { *(.rel.ctors)	}
  .rela.ctors    : { *(.rela.ctors)	}
  .rel.dtors     : { *(.rel.dtors)	}
  .rela.dtors    : { *(.rela.dtors)	}
  .rel.init      : { *(.rel.init)	}
  .rela.init     : { *(.rela.init)	}
  .rel.fini      : { *(.rel.fini)	}
  .rela.fini     : { *(.rela.fini)	}
  .rel.bss       : { *(.rel.bss)		}
  .rela.bss      : { *(.rela.bss)		}
  .rel.plt       : { *(.rel.plt)		}
  .rela.plt      : { *(.rela.plt)		}
  .init          : { *(.init)	} =0
  .plt      : { *(.plt)	}
/* .gnu.warning sections are handled specially by elf32.em.  */
  .text      :
  {
    *(.text)
    *(.stub)
    *(.gnu.warning)
    *(.gnu.linkonce.t*)
  } =0
  _etext = .;
  PROVIDE (etext = .);
  .fini      : { *(.fini)    } =0
  .rodata    : { *(.rodata) *(.gnu.linkonce.r*) }
  .rodata1   : { *(.rodata1) }
  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */
  . = ALIGN(128) + (. & (128 - 1));
  .data    :
  {
    *(.data)
    *(.gnu.linkonce.d*)
    CONSTRUCTORS
  }
  .data1   : { *(.data1) }
  . = ALIGN(4);
  .ctors         :
  {
    ___ctors = .;
    *(.ctors)
    ___ctors_end = .;
  }
  . = ALIGN(4);
  .dtors         :
  {
    ___dtors = .;
    *(.dtors)
    ___dtors_end = .;
  }
  .got           : { *(.got.plt) *(.got) }
  .dynamic       : { *(.dynamic) }
  /* We want the small data sections together, so single-instruction offsets
     can access them all, and initialized data all before uninitialized, so
     we can shorten the on-disk segment size.  */
  .sdata     : { *(.sdata) }
  _edata  =  .;
  PROVIDE (edata = .);
  __bss_start = .;
  .sbss      : { *(.sbss) *(.scommon) }
  .bss       :
  {
   *(.dynbss)
   *(.bss)
   *(COMMON)
  }
  _end = . ;

  . = ALIGN(4);
  __START_BSG = .;
  BSG : { *(BSG) }
  __END_BSG = .;

  . = ALIGN(32);
  __START_BSG32 = .;
  BSG32 : { *(BSG32) }
  __END_BSG32 = .;

/*  __BSG_END = . ;*/

  PROVIDE (end = .);
  /* Stabs debugging sections.  */
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  .stab.excl 0 : { *(.stab.excl) }
  .stab.exclstr 0 : { *(.stab.exclstr) }
  .stab.index 0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 : { *(.comment) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }

  .stack __END_BSG32 + 0x20000 : { _stack = .; *(.stack) }       /* 128 offset for stack to grow. */

  /* These must appear regardless of  .  */
}