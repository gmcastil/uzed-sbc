-- Hardware model of a 256K (32K x 8) parallel synchronou EEPROM, similar to the
-- Atmel AT28C256 EEPROM
--
-- This module emulates a 32K x 8 ROM with the expected asynchronous interface that one would find
-- in a processor, but with a secondary interface sufficient to be read and written by a block RAM
-- controller. The intent is that a processor model, such as the 6502 can use it as an EEPROM from
-- one interface, but it can be programmed, written to, purged, etc. from a higher level of abstraction
-- such as software running on a processor via a block RAM controller. An alternate method which
-- might be useful in a pure FPGA design, would be a JTAG to AXI bridge, which would allow
-- loading of the EEPROM image via JTAG.
--
-- Notes on use:
--   - The expected use is with a PS communicating via an AXI BRAM controller on one side and a 6502
--     emulator driving the SBC side of the memory, as if it were an asynchronous EEPROM
--   - The AXI BRAM controller, which is usually created via IP Integrator, needs to be configured
--     to expect a minimum read latency of 3 clock cycles. The default value for that IP is 1 so if
--     this is not set properly, there is a good chance that data read back will be incorrect.

library ieee;
use ieee.std_logic_1164.all;

library UNISIM;
use UNISIM.vcomponents.all;

entity at28c256 is
    port (
        -- PS memory controller interface
        ps_clk          : in    std_logic;
        ps_rst          : in    std_logic;
        ps_en           : in    std_logic;
        ps_we           : in    std_logic_vector(3 downto 0);
        ps_addr         : in    std_logic_vector(12 downto 0);
        ps_wr_data      : in    std_logic_vector(31 downto 0);
        ps_rd_data      : out   std_logic_vector(31 downto 0);

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
        sbc_rd_data     : out   std_logic_vector(7 downto 0)
    );
end entity at28c256;

architecture structural of at28c256 is

    signal douta    : std_logic_vector(7 downto 0);

begin

    -- Using these signals in this manner incurs an extra cycle of latency to accesses from
    -- fabric logic (access from the PS is unaffected). An alternate choice here would be to
    -- make this unclocked and incur some additional routing delay. The actual EEPROM puts the
    -- outputs in a high impedance state because in a real application, the data bus is shared.
    sbc_read: process (sbc_clk) is
    begin
        if rising_edge(sbc_clk) then
            if (sbc_web = '1') and (sbc_ceb = '0') and (sbc_oeb = '0') then
                sbc_rd_data     <= douta;
            end if;
        end if;
    end process sbc_read;

    -- True dual port RAM with independent clocks, three cycles of read latency and configured for a
    -- Port A:                                              Port B:
    --   8-bit data                                           32-bit data
    --   15-bit addr                                          13-bit addr
    --   32K depth                                            8K depth
    --
    -- Primitives output register (block RAM register)
    -- Core output register (fabric register)
    -- No reset pin (unused if provided)
    -- Write first operating mode
    --
    -- Note that the block RAM needs to be configured with 8-bit byte sizes and with the byte write
    -- enable option selected (otherwise, the port sizes will not match when the B side is hooked up
    -- to the AXI BRAM controller).
    --
    -- The PS side is brought up to the top so that it can be connected directly to the PS. Note
    -- that since we are emulating a ROM, we disable the write feature from the SBC side to prevent
    -- inadvertent data corruption.
    trp_bram_32kx8_i0: entity work.tdp_bram_32kx8
    port map (
        clka    => sbc_clk,
        ena     => '1',
        wea     => (others=>'0'),
        addra   => sbc_addr,
        dina    => (others=>'0'),
        douta   => douta,
        clkb    => ps_clk,
        enb     => ps_en,
        web     => ps_we,
        addrb   => ps_addr,
        dinb    => ps_wr_data,
        doutb   => ps_rd_data
    );

end architecture structural;
