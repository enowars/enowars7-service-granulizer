SECTIONS
{
  .mySegment1 0x20000000 : {KEEP(*(.d0))}
  .mySegment2 0x20000008 : {KEEP(*(.d1))}
}
