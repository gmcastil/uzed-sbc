# Clocking
Since my goal is to eventually make this serve as a platform building NES games
and probably clone the PPU to use as a video display device, it made sense to me
to model my clock resources the same way that the original NES did.

Per [this link](https://www.nesdev.org/wiki/Cycle_reference_chart) on the NES
developers wiki, it appears that the clocks needed are going to be the
following:

* Master Clock defined as 236.25MHz / 11 which works out to 21.477272MHz +/- 40Hz.
To simply matters, I'm going to refer to this undivided clock as the principal clock and then
derive the master clock and all others from it (for reasons that will become clear later).
* CPU clock defined as the master clock / 12, or alternatively, the principal
clock divided by 132.
* PPU clock defined as the master clock / 4 or the principal clock divied by 44.
* PPU and CPU clocks need to maintain their relationship to each other. Their
  relationship to the master clock does not appear to be significant.

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

Another thing to add here, after reading some forum posts, questions, and a
couple of useful links, notably
[this](http://www.markharvey.info/art/srldiv_04.10.2015/srldiv_04.10.2015.html)
its become clear to me that there are at least three solutions to this problem:

1. Use an MMCM / PLL combination to generate all of the frequencies that are
desired as separate (but possibly related) domains, dealing with whatever
domain crossing issues arise as one normally would (e.g., FIFO for getting
data from one domain to another, sequential flip flops for control signals,
with some handshaking logic)
2. Using a shift register to drive a pin that generates a clock divided pulse,
by preloading it with a single bit set and then feeding the output of the
shift register back into itself. The idea is that the output of this would
then be fed into a clocking resource like a BUFG or something of that nature
3. There are primitives like the BUFR that in principle can be used for some
division functions, but there are some limitations and, as a very helpful
individual on the AMD / Xilinx forums pointed out, BUFR are not general
purpose clocking resources and are really only intended to support ISERDES
and OSERDES based IO interfaces.  Very helpful.  In fact, I"m just going to
reproduce it here, since the AMD website is prone to dropping Xilinx forum
content

> For either of the two above solutions (using a fabric divider driven out on a
> BUFG or using a BUFR) there are issues. Both will generate a valid periodic
> signal and on a dedicated clock network, but both have the same problem; the
> phase difference between the divided clock and the base clock will be large
> and will vary over process, voltage, and temperature as well as from
> implementation run to implementation run. Both require the base clock to leave
> the dedicated clock network and then get back on a different one after some
> fabric routing and/or fabric resources.
>
> Furthermore, the BUFR solution has the added disadvantage that the resulting
> clock is only usable in one clock region... So while neither of these
> solutions is great, the BUFR doesn't offer any advantage over the fabric
> divider; the BUFR is really intended only to support ISERDES and OSERDES based
> I/O interfaces, and shouldn't be treated as a "general purpose" clock buffer.

> In UltraScale/UltraScale\+/Versal, the `BUFGCE_DIV` can do what you are asking
> and, while it, too, is really intended to support ISERDES/OSERDES interfaces,
> it is a "general purpose" clock buffer, and generates a clock that is on
> global clock resources.

Also, this part on the significance of not creating a 50% internal duty cycle
clock was particularly helpful, since I had not seen anyone offer an explanation
as to why SRLC32 primitives were being one-hot initialized instead of 50/50.

> But the question is always why do you need a divided clock with 50% duty
> cycle. If the clock is internal the duty cycle is irrelevant (unless you are
> also using the falling edge). If the clock is only going out of the FPGA, then
> you don't need a "true" clock on a clock network, and the periodic signal can
> be generated at the IOB (using an IOB flip-flop or ODDR). If it needs to do
> both (be used internally and go out) then there are solutions that use a
> combination of these two solutions.

This latter part actually has me thinking, because I can imagine that situations
in the future might arise where I need to bring a divided clock outside the chip
to drive peripherals.  I'm going to put this in the _problem that
I'm going to need to solve at a later date_ category.  A situation I can imagine
is that I want to drive a SPI interface or something like that which actually
leaves the chip and heads to one of PMOD connectors.  But as stated, that is a
problem for a later date.

4. The solution that seems best to me is to use fabric resources from a single
domain to create a clock enable that strobes the clock pins. This has a
couple of advantages.  First, and most importantly, it allows me to create CPU
and PPU clock domains that remain in phase between implementation runs. The
original NES (which I'm basically stealing for sound, video, and probably some
IO) required a 3:1 fixed relationship between those two clock domains.  I need
to maintain that relationship and the easiest way is to just generate clock
enables that always and forever have that relationship because they're on the
same domain.  That leads to the second point, in that all my derived clocks are
going to be on the same domain and I never have to deal with domain crossings
between the CPU and PPU domains (or anything else for that matter which is
sensitive to the same clock). Third, and the previous post really clarifies that
a lot, the BUFR solution which supports some integeter division (notably, not
the 11 that I need), is a local resource, so I can't spread it across the chip.
That's in keeping with its role as a clock source for IO interfaces. Some
applications may be ok with this, but in my case, I'm quite likely going to want
to spread my logic across more than one clocking region and running into massive
congestion is not a problem I want to have to deal with. 


