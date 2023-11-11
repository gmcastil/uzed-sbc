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

After a morning of reading and experimenting, I realized that I have 
