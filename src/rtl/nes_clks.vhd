library ieee;
use ieee.std_logic_1164.all;

library UNISIM;
use UNISIM.vcomponents.all;

entity nes_clks is
    generic (
        -- If desired, an IBUF can be inserted on either the input clock
        -- or external reset signals. No BUFG is required or should be included
        -- on the input clock, unless external logic needs to run on that
        -- domain.
        CLK_IN_IBUF             : boolean   := false;
        RST_IN_IBUF             : boolean   := false;
        -- Number of `clk_ref` ticks before `rst_ref` is deasserted
        REF_RST_LENGTH          : natural   := 10;
        -- Number of `clk_mst` ticks before `rst_ref` is deasserted
        MST_RST_LENGTH          : natural   := 10;
        -- Number of `clk_en_ppu` ticks before `rst_en_ppu` is deasserted
        PPU_EN_RST_LENGTH       : natural   := 4;
        -- Number of `clk_en_cpu` ticks before `rst_en_cpu` is deasserted
        CPU_EN_RST_LENGTH       : natural   := 4
    );
    port (
        -- Nominally a 100MHz input clock, but other values may be possible,
        -- with modifications to the MMCM configuration.  Internally, we
        -- first generate a 236.25MHz clock, divide by 11 to get the 21.477MHz
        -- master clock and the CPU and PPU clock enables from this.
        clk_ext         : in    std_logic;
        -- External active-high reset tied directly to the MMCM reset input.
        rst_ext         : in    std_logic;

        -- 236.25MHz clock synthesized from 100MHz input
        clk_ref         : out   std_logic;
        -- 21.477MHz master clock divided by 11 from the 236.25MHz clock
        clk_mst         : out   std_logic;
        -- PPU clock enable created by dividing the master clock by 5
        clk_en_ppu      : out   std_logic;
        -- CPU clock enable created by dividing the master clock by 12
        clk_en_cpu      : out   std_logic;
        
        -- Active low reset synchronous to 236.25MHz clock
        rst_ref         : out   std_logic;
        -- Active low reset synchronous to 21.477MHz master clock. 
        rst_mst         : out   std_logic;
        -- Active low resets synchronous to the 21.477MHz master clock, but
        -- timed to deassert coincident with the rising edge of the PPU and
        -- CPU clock enables. It is recommended that circuits that use the
        -- divided clock enables are reset with these signals.
        rst_en_ppu      : out   std_logic;
        rst_en_cpu      : out   std_logic
    );
end nes_clks;

architecture structural of nes_clks is

    signal  clk_mmcm        : std_logic;
    signal  rst_mmcm        : std_logic;

    signal  clk_236m25      : std_logic;
    signal  clk_21m477      : std_logic;
    signal  clk_fb          : std_logic;
    signal  mmcm_locked     : std_logic;

begin

    -- If indicated, add IBUF to the input clock or reset
    clk_buffer : if CLK_IN_IBUF generate
    begin
        IBUF_clk_ext : IBUF
        generic map (
            IBUF_LOW_PWR    => TRUE,
            IOSTANDARD      => "DEFAULT"
        )
        port map (
            O               => clk_mmcm,
            I               => clk_ext
        );
    else generate
        clk_mmcm            => clk_ext;
    end generate

    rst_buffer : if RST_IN_IBUF generate
    begin
        IBUF_rst_ext : IBUF
        generic map (
            IBUF_LOW_PWR    => TRUE,
            IOSTANDARD      => "DEFAULT"
        )
        port map (
            O               => rst_mmcm;
            I               => rst_ext
        );
    else generate
        rst_mmcm            => rst_ext;
    end generate

    -- Synthesize the 236.25 and 21.477 MHz internal clocks from the 100MHz
    -- input clock
    MMCME2_ADV_i0 : MMCME2_ADV
    generic map (
        BANDWIDTH               => "OPTIMIZED",
        CLKFBOUT_MULT_F         => 47.250,
        CLKFBOUT_PHASE          => 0.0,
        -- CLKIN_PERIOD: Input clock period in ns to ps resolution (i.e. 33.333 is 30 MHz).
        CLKIN1_PERIOD           => 10.0,
        CLKIN2_PERIOD           => 0.0,
        -- CLKOUT0_DIVIDE - CLKOUT6_DIVIDE: Divide amount for CLKOUT (1-128)
        CLKOUT1_DIVIDE          => 44,
        CLKOUT2_DIVIDE          => 1,
        CLKOUT3_DIVIDE          => 1,
        CLKOUT4_DIVIDE          => 1,
        CLKOUT5_DIVIDE          => 1,
        CLKOUT6_DIVIDE          => 1,
        CLKOUT0_DIVIDE_F        => 4.0,
        -- CLKOUT0_DUTY_CYCLE - CLKOUT6_DUTY_CYCLE: Duty cycle for CLKOUT outputs (0.01-0.99).
        CLKOUT0_DUTY_CYCLE      => 0.5,
        CLKOUT1_DUTY_CYCLE      => 0.5,
        CLKOUT2_DUTY_CYCLE      => 0.5,
        CLKOUT3_DUTY_CYCLE      => 0.5,
        CLKOUT4_DUTY_CYCLE      => 0.5,
        CLKOUT5_DUTY_CYCLE      => 0.5,
        CLKOUT6_DUTY_CYCLE      => 0.5,
        -- CLKOUT0_PHASE - CLKOUT6_PHASE: Phase offset for CLKOUT outputs (-360.000-360.000).
        CLKOUT0_PHASE           => 0.0,
        CLKOUT1_PHASE           => 0.0,
        CLKOUT2_PHASE           => 0.0,
        CLKOUT3_PHASE           => 0.0,
        CLKOUT4_PHASE           => 0.0,
        CLKOUT5_PHASE           => 0.0,
        CLKOUT6_PHASE           => 0.0,
        CLKOUT4_CASCADE         => FALSE,
        COMPENSATION            => "ZHOLD",
        DIVCLK_DIVIDE           => 5,
        -- REF_JITTER: Reference input jitter in UI (0.000-0.999).
        REF_JITTER1             => 0.0,
        REF_JITTER2             => 0.0,
        STARTUP_WAIT            => FALSE,
        -- Spread Spectrum: Spread Spectrum Attributes
        SS_EN                   => "FALSE",
        SS_MODE                 => "CENTER_HIGH",
        SS_MOD_PERIOD           => 10000,
        -- USE_FINE_PS: Fine phase shift enable (TRUE/FALSE)
        CLKFBOUT_USE_FINE_PS    => FALSE,
        CLKOUT0_USE_FINE_PS     => FALSE,
        CLKOUT1_USE_FINE_PS     => FALSE,
        CLKOUT2_USE_FINE_PS     => FALSE,
        CLKOUT3_USE_FINE_PS     => FALSE,
        CLKOUT4_USE_FINE_PS     => FALSE,
        CLKOUT5_USE_FINE_PS     => FALSE,
        CLKOUT6_USE_FINE_PS     => FALSE
    )
    port map (
        -- Clock Outputs: 1-bit (each) output: User configurable clock outputs
        CLKOUT0                 => clk_236m25,
        CLKOUT0B                => open,
        CLKOUT1                 => clk_21m477,
        CLKOUT1B                => open,
        CLKOUT2                 => open,
        CLKOUT2B                => open,
        CLKOUT3                 => open,
        CLKOUT3B                => open,
        CLKOUT4                 => open,
        CLKOUT5                 => open,
        CLKOUT6                 => open,
        -- DRP Ports: 16-bit (each) output: Dynamic reconfiguration ports
        DO                      => open,
        DRDY                    => open,
        -- Dynamic Phase Shift Ports: 1-bit (each) output: Ports used for dynamic phase shifting of the outputs
        PSDONE                  => open,
        -- Feedback Clocks: 1-bit (each) output: Clock feedback ports
        CLKFBOUT                => clk_fb,
        CLKFBOUTB               => open,
        -- Status Ports: 1-bit (each) output: MMCM status ports
        CLKFBSTOPPED            => open,
        CLKINSTOPPED            => open,
        LOCKED                  => mmcm_locked,
        -- Clock Inputs: 1-bit (each) input: Clock inputs
        CLKIN1                  => clk_mmcm,
        CLKIN2                  => '0',
        -- Control Ports: 1-bit (each) input: MMCM control ports
        CLKINSEL                => '1',
        PWRDWN                  => '0',
        RST                     => rst_mmcm,
        -- DRP Ports: 7-bit (each) input: Dynamic reconfiguration ports
        DADDR                   => (others=>'0'),
        DCLK                    => '0',
        DEN                     => '0',
        DI                      => (others=>'0'),
        DWE                     => '0',
        -- Dynamic Phase Shift Ports: 1-bit (each) input: Ports used for dynamic phase shifting of the outputs
        PSCLK                   => '0',
        PSEN                    => '0',
        PSINCDEC                => '0',
        -- Feedback Clocks: 1-bit (each) input: Clock feedback ports
        CLKFBIN                 => clk_fb
        );

        -- Add the two synthesized clocks to the global clock network

end structural;

