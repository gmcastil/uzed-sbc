library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity safe_start is

    generic (
        ACTIVE_LOW          : boolean   := true,
        RST_LENGTH          : natural   := 8
    );
    port (
        -- Input clock should not be driven by anything other than an IBUF. Do
        -- NOT insert a BUFG or BUFGCE or any other type of clock buffer. This
        -- should almost certainly be driven by an external oscillator, fabric
        -- clock, or MMCM / PLL output.
        raw_clk     : in    std_logic;
        -- Asynchronous input reset must be active low. If an active high input
        -- reset is desired, it can be inverted prior to connection with no
        -- effect upon timing.
        arst        : in    std_logic;

        -- Safely buffered clock on the global clock distribution network that
        -- can be used anywhere
        safe_clk    : out   std_logic;
        -- Reset synchronous to the buffered clock output
        sync_rst    : out   std_logic
    );
        
end entity safe_start;

architecture struct of safe_start is

    signal clk_local        : std_logic;
    signal rst_chain        : std_logic_vector(0 to RST_LENGTH);

begin

    -- Create a local clock for low fanout, internal use. Note that we do not
    -- use this clock for anything other than generating an internal reset
    -- signal
    BUFH_i0: BUFH
    port map (
        O       => clk_local,
        I       => raw_clk
    );

    -- Instantiate a chain of FDCE (active low) or FDPE (active high) to drive
    -- the output clock enable and synchronous output reset
    g_active_low: if ACTIVE_LOW generate
    begin
        rst_chain(0)    <= '1';
        g_rst_chain: for i in 0 to (rst_chain'right - 1) generate
            FDCE_i: FDCE
            generic map (
                INIT    => '0'
            )
            port map (
                Q       => rst_chain(i+1),
                C       => clk_local,
                CE      => '1',
                CLR     => not arst,
                D       => rst_chain(i)
            );
        end generate
    else generate
    begin
        rst_chain(0)    <= '1';
        g_rst_chain: for i in 0 to (rst_chain'right - 1) generate
            FDPE_i: FDPE
            generic map (
                INIT    => '1'
            )
            port map (
                Q       => rst_chain(i+1),
                C       => clk_local,
                CE      => '1',
                CLR     => not arst,
                D       => rst_chain(i)
            );
        end generate
    end generate

    -- Synchronous reset is just the output of the chain of flip flops
    sync_rst            <= rst_chain(rst_chain'right);
    -- Output clock is the input clock, but buffered and enabled by the local
    -- reset that was created. Suble but important detail here is that the
    -- BUFGCE here is NOT driven by the local clock.  BUFH -> BUFG is bad.
    BUFGCE_i0: BUFGCE
    port map (
        O       => safe_clk,
        CE      => rst_chain(rst_chain'right),
        I       => raw_clk
    );

end architecture struct;

