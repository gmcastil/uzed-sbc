-- Hardware model of a 256K (32K x 8) parallel synchronou EEPROM, similar to the
-- Atmel AT28C256 EEPROM
--
-- This module emulates a 32K x 8 ROM with the expected asynchronous interface that one would find
-- in a processor, but with a secondary interface sufficient to be read and written by a block RAM
-- controller. The intent is that a processor model, such as the 6502 can use it as an EEPROM from
-- one interface, but it can be programmed, written to, purged, etc. from a higher level of abstraction
-- such as software running on a processor via a block RAM controller. 

library ieee;
use ieee.std_logic_1164.all;

entity at28c256
    port (
        -- PS memory controller interface
        ps_clk			: in    std_logic;
        ps_en			: in    std_logic;
        ps_we			: in    std_logic;
        ps_addr		    : in    std_logic_vector(14 downto 0);
        ps_din			: in    std_logic_vector(7 downto 0);
        ps_dout			: in    std_logic_vector(7 downto 0);

        -- The SBC processor sees this as an asynchronous memory, so we supply a clock that is
        -- synchronous to the SBC memory interface signals, but sufficiently fast such that
        -- accesses are completed in the expected interval.
        sbc_clk         : in    std_logic;
        -- Depending upon how the address decoder logic of the processor is structured, all or less
        -- of the EEPROM emulator can be used. A common application might be to only connect up the
        -- first 14 bits of the EEPROM to get 16KB of ROM.
        sbc_addr        : in    std_logic_vector(14 downto 0);
        -- The control signals are not simply wired to block RAM.
        sbc_ceb         : in    std_logic;
        sbc_oeb         : in    std_logic;
        -- Writes to the EEPROM are not supported, but this signal does need to be driven
        -- appropriately.
        sbc_web         : in    std_logic;
        sbc_rd_data     : out   std_logic_vector(7 downto 0);
    );

end entity at28c256;

architecture struct of at28c256 is

    signal doutb    : std_logic_vector(7 downto 0);

begin

    -- Using these signals in this manner incurs an extra cycle of latency to accesses from
    -- fabric logic (access from the PS is unaffected). An alternate choice here would be to 
    -- make this unclocked and incur some additional routing delay. The actual EEPROM puts the
    -- outputs in a high impedance state because in a real application, the data bus is shared.
    sbc_read: process(sbc_clk) is
    begin
        if sbc_web and not sbc_ceb and not sbc_oeb then
            sbc_rd_data     <= doutb;
        else
            sbc_rd_data     <= sbc_rd_data;
        end if;

    end process sbc_read;

    -- True dual port RAM with independent clocks, three cycles of read latency and configured for a
    -- depth of 32K and width of 8-bits. The PS side is brought up to the top so that it can be
    -- connected directly to the PS. Note that since we are emulating a ROM, we disable the write
    -- feature from the SBC side to prevent inadvertent data corruption.
    trp_bram_32kx8_i0: tdp_bram_32kx8
    port map (
        clka	=> ps_clk,
        ena		=> ps_en,
        wea		=> ps_we,
        addra	=> ps_addr,
        dina	=> ps_din,
        douta	=> ps_dout,
        clkb	=> sbc_clk,
        enb		=> '1',
        web		=> '0',
        addrb	=> sbc_addrb,
        dinb	=> open,
        doutb	=> doutb
    );

end architecture struct;
