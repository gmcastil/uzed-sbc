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
use ieee.numeric_std.all;

library UNISIM;
use UNISIM.vcomponents.all;

entity at28c256 is
    generic (
        -- When true, replace the PL side memory interface with an ILA and VIO that allows
        -- manual dumping of memory pages (possibly useful for debugging PS side AXI access)
        DEBUG_PAGE_DUMP_ENABLED     : boolean   := false;
        BRAM_RD_LATENCY             : integer   := 3
    )
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

    signal douta            : std_logic_vector(7 downto 0);

    -- We wish to add the ability to dump a 4KB page of the block RAM from the hardware side to
    -- chipscope if a VIO and ILA are enabled during synthesis.
    signal dump_start       : std_logic;
    signal dump_page        : unsigned(2 downto 0);
    signal dump_cnt         : unsigned(12 downto 0);
    signal dump_addr        : std_logic(14 downto 0);
    signal dump_rd_stall    : unsigned(2 downto 0);

    component tdp_bram_32kx8
    port (
        clka                : in    std_logic;
        ena                 : in    std_logic;
        wea                 : in    std_logic_vector(0 downto 0);
        addra               : in    std_logic_vector(14 downto 0);
        dina                : in    std_logic_vector(7 downto 0);
        douta               : out   std_logic_vector(7 downto 0);
        clkb                : in    std_logic;
        enb                 : in    std_logic;
        web                 : in    std_logic_vector(3 downto 0);
        addrb               : in    std_logic_vector(12 downto 0);
        dinb                : in    std_logic_vector(31 downto 0);
        doutb               : out   std_logic_vector(31 downto 0)
    );
    end component;

    component vio_dump_page_ctrl
    port (
        clk                 : in    std_logic;
        probe_out0          : out   std_logic_vector(0 downto 0);
        probe_out1          : out   std_logic_vector(2 downto 0)
    );
    end component;


begin

    -- This logic is straight out of the AT28C256 datasheet, but in our case with registered outputs
    -- and an extra cycle of latency. It is expected that the processor is only looking at the ROM
    -- during clock cycles when the CPU clock enable is asserted, which is /12 the SBC clock rate.
    -- To make it easier to detect problems, we drive the data bus with all ones unless it is
    -- enabled properly (even though we have the data from the memory).
    sbc_read: process (sbc_clk) is
    begin
        if rising_edge(sbc_clk) then
            if (sbc_web = '1') and (sbc_ceb = '0') and (sbc_oeb = '0') then
                sbc_rd_data     <= pl_rd_data;
            else
                sbc_rd_data     <= (others=>'1');
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
        ena     => pl_en,
        wea     => (others=>'0'),
        addra   => pl_addr,
        dina    => (others=>'0'),
        douta   => pl_rd_data
        clkb    => ps_clk,
        enb     => ps_en,
        web     => ps_we,
        addrb   => ps_addr,
        dinb    => ps_wr_data,
        doutb   => ps_rd_data
    );

    if DEBUG_PAGE_DUMP_ENABLED generate
        -- Instantiate a VIO
        dump_page_ctrl: vio_dump_page_ctrl
        port map (
            clk i       => sbc_clk,
            probe_out0  => dump_start,
            probe_out1  => dump_page
        );

        -- Need a rising edge detector to actually initiaiate the block RAM dump
        dump_start_red: process(sbc_clk)
        begin
            if rising_edge(sbc_clk) then
                dump_start_q        <= dump_start;
                dump_start_qq       <= dump_start_q;
                if (dump_start_qq = '0' and dump_start_q = '1') then
                    dump_start_red  <= '1';
                else
                    dump_start_red  <= '0';
                end if;
            end if;
        end process dump_start_red;

        -- The dump address is always just the concatenation of the page we're dumping
        -- and the counter value that we're incrementing
        dump_addr           <= std_logic_vector(dump_page) & std_logic_vector(dump_cnt);

        -- Now we need to increment the counter after the start indicator is given and then
        -- wait the appropriate number of clocks to let the BRAM service the read request
        dump_page: process(sbc_clk)
        begin
            if rising_edge(clk) then
                if (dump_start_red = '1' and dump_busy = '0') then
                    dump_busy       <= '1';
                    dump_rd_stall   <= 0;
                elsif (dump_busy = '1') then
                    -- Data should have arrived
                    if (dump_rd_stall != BRAM_RD_LATENCY) then


                    -- Still need to wait for BRAM to service request
                        else

                else

                end if;
            end if;
        end process dump_page;

        -- Instantiate an ILA
        dump_page_mon: ila_dump_page_mon 
        port map (
            clk         => sbc_clk,
            probe0      => dump_start_red,
            probe1      => dump_addr,
            probe2      => dump_busy,
            probe3      => dump_rd_stall,
            probe3      => pl_rd_data
        );


    else generate
        -- When we're not debugging this interface, we just wire the enable
        -- high all the time and the address at the BRAM is the same as the address
        -- from the SBC.
        pl_addr     <= sbc_addr;
        pl_en       <= '1';
    end generate

end architecture structural;
