Memory access has really been confusing, since I have to hold two completely
independent ways of looking at memory in my head at the same time.

From a physical perspective, I now have two 32KB memories available to me at two
different locations and from the PS side, they're the same. I can read and write
to either of them, regardless of how they are hooked up in the PL (e.g., one
might be emulating a ROM and not be writable and the other a RAM, but only have
half the address lines hooked up and be exposed as a 16KB SRAM).

Where it gets confusing to me is in the AXI to BRAM control bridge (and I'm
still not convinced I actually have it understood yet and won't until I build a
hardware tester which dumps the block RAM in hardware from the PL side.  In
fact, I might actually make one of those tomorrow, then put it in hardware,
dump it, and then capture it in chipscope and see what is actually coming out
and how its arranged on the address bus.

That said, the AXI address editor in Vivado expresses the block RAM as an 8K
range with the understanding that each address is actually a 32-bit value.  But,
at the block RAM, individual bytes can be changed and that's done by having a
byte-enable bit.  The current suite of tools I have for doing block RAM access
and so forth treat the memory array, once it is mapped, as a flexible space that
can be interpreted in different ways (and that may still be a possibility). But,
I have code that is interpreting the addresses incorrectly I think.

Couple steps:
- Build a memory dump tool that I can stick on the PL side of the block RAM,
  hook up to the address line, and then dump the entire thing in hardware to
chipscope.
- This lets me verify the endianness, the orientation, and so forth of the
  memory so that I can see that what I think I'm writing is actually getting
written to and in the right spot
- Probably hook up a VIO and ILA to it as well - make a real memory diagnostic
  RTL piece.
- Also lets me test my clock generation logic too
- Then once I have a better idea of what is actually happening to the PL side of
  the block RAM (the part I care about) then I can go back and refactor my block
  RAM control tools.

I knew I was probably going to need to do this sort of thing, this isn't
unexpected.

