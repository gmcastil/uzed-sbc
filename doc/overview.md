Block Design
------------

The single board computer requires a few things before instantiating and working
on the processor becomes a thing.

- EEPROM and RAM interfaces to software and hardware
- Clocks and reset
- Command, control, and monitoring
- Domain crossing from the sbc domain to the monitoring domain

These last three are somewhat related and need some careful thought to avoid
inroducing problems that are going to be a pain to deal with later.  The entire
thing really revolves around the clocking architecture being set up correctly.

Clocking Requirements
---------------------

- Need to be able to put the sbc clock in a free running mode where it just runs
  after being commanded to start
- I want to be able to set a value in a register, then hit a start bit, and then
  generate that many pulses on the SBC clock domain.
- I also want to be able to hit a pulse bit which single steps the output clock
  one pulse
- The SBC clock is going to be the same 
    21.477272 MHz +- 40 Hz
    236.25 MHz / 11 by definition
    21.47~ MHz ÷ 12 = 1.789773 MHz 
- So, we're going to be targeting this 1.789773 MHz as the primary SBC clock,
  which is very slow, and then we'll produce an SBC clock x4 for the PPU:w




