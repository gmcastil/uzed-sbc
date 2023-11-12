# Clocking
Since my goal is to eventually make this serve as a platform building NES games
and probably clone the PPU to use as a video display device, it made sense to me
to model my clock resources the same way that the original NES did.

Per [this link](https://www.nesdev.org/wiki/Cycle_reference_chart) on the NES
developers wiki, it appears that the clocks needed are going to be the
following:

Master Clock defined as 236.25MHz / 11 which works out to 21.477272MHz +/- 40Hz.
To simply matters, I'm going to refer to this undivided clock as the principal clock and then
derive the master clock and all others from it (for reasons that will become clear later).

CPU clock defined as the master clock / 12, or alternatively, the principal
clock divided by 132.

PPU clock defined as the master clock / 4 or the principal clock divied by 44.

To emulate the asynchronous memory that 6502 based platforms have, I will need
an additional clock to overclock the PL side of the block RAM.  For each rising
edge of the CPU clock, I will need to have a certain number of additional clock
cycles to allow the asynchronous memory emulator (i.e., the block RAM to clock
data in and out) so that from the CPU clock domain, memory access seems
asynchronous, but the block RAM interface remains completely synchronous.  It
may be wise to create some sort of checking circuit that verifies the
relationship doesn't begin to drift.

At first, I thought I would jsut use an MMCM, plug in a sufficient input clock,
determine my values, and then be done.  This turned out to be extraordinarily
naive - I had never really sat down and read the documentation on the MMCM and
PLL primitives before.  I'm glad I ran into this problem, because it gave me the
opportunity to wrap my mind around the care and feeding of those elements.

The basic requirements are the following:
- 100MHz input clock frequency for two reasons.  First, it's easily accessible
  from the fabric clock network and doesn't require altering the PS
  configuration to achieve.  Second, the IO carrier card that I have and will
  likely migrate to for development on the 7020 has an onboard 100MHz oscillator
  and I like the idea of being able to use that if I need to.
- From the 100MHz we're going to generate the 236.25MHz principal clock for the
  PL side of the design using a single MMCM and the first clock output
- Then, using a sequence of clock dividers based on shift registers, I'm going
  to directly divide the principal clock into the master, PPU, CPU, and memory
  clocks.  I'm also going to directly instantiate all the clocking primitives,
  flops, and reset signals in hardware (I was going to have a single clock and
  reset module, but this is probably going to get split up).

I'm going to get started on the MMCM instantiation first, see if I can get the
master clock created, and then see if it gets through the design rule checks.

