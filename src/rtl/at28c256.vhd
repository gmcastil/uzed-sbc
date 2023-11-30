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
        DEBUG_PAGE_DUMP_ENABLED     : boolean               := false;
        BRAM_RD_LATENCY             : unsigned(3 downto 0)  := x"3"
    );
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

    constant PL_ADDR_IDLE   : std_logic_vector(14 downto 0) := b"111111111111111";

    signal pl_en            : std_logic;
    signal pl_addr          : std_logic_vector(14 downto 0);
    signal pl_rd_data       : std_logic_vector(7 downto 0);

    signal dump_start       : std_logic_vector(0 downto 0);
    signal dump_start_q     : std_logic;
    signal dump_start_qq    : std_logic;
    signal dump_start_red   : std_logic;
                              
    signal dump_busy        : std_logic;
    signal rd_stall_cnt     : unsigned(3 downto 0);
    signal dump_page        : std_logic_vector(6 downto 0);
    signal addr_cnt         : unsigned(7 downto 0);

    attribute MARK_DEBUG                    : string;
    attribute MARK_DEBUG of pl_en			: signal is "TRUE";
    attribute MARK_DEBUG of pl_addr			: signal is "TRUE";
    attribute MARK_DEBUG of pl_rd_data		: signal is "TRUE";
    attribute MARK_DEBUG of dump_start		: signal is "TRUE";
    attribute MARK_DEBUG of dump_start_q	: signal is "TRUE";
    attribute MARK_DEBUG of dump_start_qq	: signal is "TRUE";
    attribute MARK_DEBUG of dump_start_red	: signal is "TRUE";
    attribute MARK_DEBUG of dump_busy		: signal is "TRUE";
    attribute MARK_DEBUG of rd_stall_cnt	: signal is "TRUE";
    attribute MARK_DEBUG of dump_page		: signal is "TRUE";
    attribute MARK_DEBUG of addr_cnt		: signal is "TRUE";

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
        probe_out1          : out   std_logic_vector(6 downto 0)
    );
    end component;

    component ila_dump_page_mon
    port (
        clk                 : in    std_logic;
        probe0              : in    std_logic_vector(0 downto 0);
        probe1              : in    std_logic_vector(0 downto 0);
        probe2              : in    std_logic_vector(6 downto 0);
        probe3              : in    std_logic_vector(3 downto 0);
        probe4              : in    std_logic_vector(7 downto 0);
        probe5              : in    std_logic_vector(0 downto 0);
        probe6              : in    std_logic_vector(14 downto 0);
        probe7              : in    std_logic_vector(7 downto 0)
    );
    end component;

begin

    -- We define two very different mechanisms for driving the block RAM. A normal read operation
    -- enables the block RAM when the control signals are set appropriately and disables it after
    -- that. The relationships between the clock frequency and the clock enable signal and the read
    -- latency need to be such that a read is completed prior to the next read request.
    bram_ctrl: if not DEBUG_PAGE_DUMP_ENABLED generate
        -- When we're not debugging this interface, the address at the BRAM is
        -- always wired into the block RAM address.
        pl_addr         <= sbc_addr;

        -- This logic is straight out of the AT28C256 datasheet, but in our case with registered
        -- outputs and an extra cycle of latency. We enable the PL side of the BRAM when the SBC
        -- control signals are correct, and then ignore read requests until finished. The busy
        -- signal is present to prevent the next read from being performed prior to completing the
        -- first (i.e., no pipelining of reads is allowed).
        sbc_read: process (sbc_clk) is
        begin
            if rising_edge(sbc_clk) then
                if (sbc_web = '1') and (sbc_ceb = '0') and (sbc_oeb = '0') and (busy = '0') then
                    pl_en           <= '1';
                    busy            <= '1';
                    rd_stall_cnt    <= BRAM_RD_LATENCY - 1;
                elsif (busy = '1') then
                    if (rd_stall_cnt /= x"0") then
                        pl_en           <= '1';
                        busy            <= '1';
                        rd_stall_cnt    <= rd_stall_cnt - 1;
                    else
                        pl_en           <= '0';
                        busy            <= '0';
                        rd_stall_cnt    <= x"0";
                        sbc_rd_data     <= pl_rd_data;
                    end if;
                else
                    pl_en           <= '0';
                    busy            <= '0';
                    rd_stall_cnt    <= x"0";
                end if;
            end if;
        end process sbc_read;

    -- If desired, we allow the block RAM to be dumped from Vivado using an internal VIO and ILA
    -- combination. A 7-bit page number can first be set and then a single start bit can be set
    -- which issues a sequence of reads that dumps the contents of that page to the ILA. This is
    -- useful when attempting to verify that the PS side of the block RAM interface (or more
    -- accurately, the AXI controller) is functioning properly. In this context, a page refers to a
    -- 256 byte address range such as 0x00FF - 0x0000 (i.e., the zero page) rather than the 4KB
    -- pages that are commonly used in systems with virtual memory, such as the Linux kernel.
    else generate

        -- Need a rising edge detector to actually initiaiate the block RAM dump
        start_dump_red_p: process(sbc_clk)
        begin
            if rising_edge(sbc_clk) then
                dump_start_q        <= dump_start(0);
                dump_start_qq       <= dump_start_q;
                if (dump_start_qq = '0' and dump_start_q = '1') then
                    dump_start_red  <= '1';
                else
                    dump_start_red  <= '0';
                end if;
            end if;
        end process start_dump_red_p;

        -- When we receive a start indicator from the VIO, we start at the beginning of the page,
        -- enable the block RAM, and then start reading one byte at a time out of the memory,
        -- waiting the required number of clocks after each read.  If we have purged the memory with
        -- an incrementing pattern, we should see a sequence of bytes in the ILA from 0x00 to 0xFF
        -- from the memory. Since we are setting the bus to PL_ADDR_IDLE after the presentation of
        -- each address, the address values in the ILA will hop all over the place, but the data
        -- should come out sequentially in a well-behaved order that is easily recognizable in the
        -- hardware manager.
        dump_page_p: process(sbc_clk)
        begin
            if rising_edge(sbc_clk) then
                if (dump_start_red = '1' and busy = '0') then
                    busy                <= '1';
                    rd_stall_cnt        <= BRAM_RD_LATENCY - 1;
                    pl_en               <= '1';
                    pl_addr             <= dump_page & addr_cnt;
                    addr_cnt            <= addr_cnt + 1;
                elsif (busy = '1') then
                    if (rd_stall_cnt /= 0) then
                        dump_busy           <= '1';
                        rd_stall_cnt        <= rd_stall_cnt - 1;
                        pl_en               <= '1';
                        pl_addr             <= PL_ADDR_IDLE;
                        addr_cnt            <= addr_cnt;
                    else
                        if (addr_cnt = x"FF") then
                            dump_busy           <= '0';
                            rd_stall_cnt        <= x"0";
                            pl_en               <= '0';
                            pl_addr             <= PL_ADDR_IDLE;
                            addr_cnt            <= x"00";
                        else
                            dump_busy           <= '1';
                            rd_stall_cnt        <= BRAM_RD_LATENCY;
                            pl_en               <= '1';
                            pl_addr             <= dump_page & std_logic_vector(addr_cnt + 1);
                            addr_cnt            <= addr_cnt + 1;
                        end if;
                    end if;
                else
                    dump_busy           <= '0';
                    rd_stall_cnt        <= x"0";
                    pl_en               <= '0';
                    pl_addr             <= PL_ADDR_IDLE;
                    addr_cnt            <= x"00";
                end if;
            end if;
        end process dump_page_p;

        -- Instantiate a VIO to initiate a dump operation for a given page
        dump_page_ctrl: vio_dump_page_ctrl
        port map (
            clk         => sbc_clk,
            probe_out0  => dump_start,
            probe_out1  => dump_page
        );

        -- Instantiate an ILA to monitor the result of dumping a page in hardware
        dump_page_mon: ila_dump_page_mon 
        port map (
            clk         => sbc_clk,
            probe0(0)   => dump_start_red,
            probe1(0)   => dump_busy,
            probe2      => dump_page,
            probe3      => std_logic_vector(rd_stall_cnt),
            probe4      => std_logic_vector(addr_cnt),
            probe5(0)   => pl_en,
            probe6      => pl_addr,
            probe7      => pl_rd_data
        );

    end generate bram_ctrl;

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
    -- inadvertent data corruption. The PL side write enable is hooked to a vector because the
    -- ECAD tool creates a degenerate case (i.e., std_logic_vector(0 downto 0)).
    trp_bram_32kx8_i0: entity tdp_bram_32kx8
    port map (
        clka    => sbc_clk,
        ena     => pl_en,
        wea     => (others=>'0'),
        addra   => pl_addr,
        dina    => (others=>'0'),
        douta   => pl_rd_data,
        clkb    => ps_clk,
        enb     => ps_en,
        web     => ps_we,
        addrb   => ps_addr,
        dinb    => ps_wr_data,
        doutb   => ps_rd_data
    );

end architecture structural;
