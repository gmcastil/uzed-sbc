library ieee;
use ieee.std_logic_1164.all;

entity sbc_core is

    generic (
        -- Platform and target
        VENDOR                  : string    := "XILINX";
        TARGET                  : string    := "7SERIES";
        -- Clock and reset configuration options
        ADD_CLK_IBUF            : boolean   := false;
        ADD_RST_IBUF            : boolean   := false;
        REF_RST_LENGTH          : natural   := 8;
        MST_RST_LENGTH          : natural   := 8;
        PPU_EN_RST_LENGTH       : natural   := 4;
        CPU_EN_RST_LENGTH       : natural   := 4
    );
    port (

        -- SBC clocks and resets
        clk_ext                 : in    std_logic;
        rst_ext                 : in    std_logic;

        -- Note that in either the EEPROM or SRAM case, the external block RAM interface can be
        -- driven from the processing subsystem of a Zynq or SoC device, or from an RTL source such
        -- as a serial port to block RAM bridge or as a JTAG to AXI slave. This allows the SBC to be
        -- used in non SoC devices or alternate vendors without the same proliferation of AXI
        -- devices as Xilinx.

        -- External EEPROM block RAM controller interface
        ext_rom_clk             : in    std_logic;
        ext_rom_rst             : in    std_logic;
        ext_rom_en              : in    std_logic;
        ext_rom_we              : in    std_logic_vector(3 downto 0);
        ext_rom_addr            : in    std_logic_vector(12 downto 0);
        ext_rom_wr_data         : in    std_logic_vector(31 downto 0);
        ext_rom_rd_data         : out   std_logic_vector(31 downto 0);

        -- External SRAM block RAM controller interface
        ext_sram_clk            : in    std_logic;
        ext_sram_rst            : in    std_logic;
        ext_sram_en             : in    std_logic;
        ext_sram_we             : in    std_logic_vector(3 downto 0);
        ext_sram_addr           : in    std_logic_vector(12 downto 0);
        ext_sram_wr_data        : in    std_logic_vector(31 downto 0);
        ext_sram_rd_data        : out   std_logic_vector(31 downto 0)
    );

end entity sbc_core;

architecture structural of sbc_core is

    -- Clock, reset, and clock enable
    signal clk_ref          : std_logic;
    signal clk_mst          : std_logic;
    signal clk_en_ppu       : std_logic;
    signal clk_en_cpu       : std_logic;
    signal rst_ref          : std_logic;
    signal rst_mst          : std_logic;
    signal rst_en_ppu       : std_logic;
    signal rst_en_cpu       : std_logic;

begin

    -- Clocks and resets
    nes_clks_i0: entity work.nes_clks
    generic map (
        ADD_CLK_IBUF        => ADD_CLK_IBUF,
        ADD_RST_IBUF        => ADD_RST_IBUF,
        REF_RST_LENGTH      => REF_RST_LENGTH,
        MST_RST_LENGTH      => MST_RST_LENGTH,
        PPU_EN_RST_LENGTH   => PPU_EN_RST_LENGTH,
        CPU_EN_RST_LENGTH   => CPU_EN_RST_LENGTH
    )
    port map (
        clk_ext             => clk_ext,         -- in    std_logic
        rst_ext             => rst_ext,         -- in    std_logic
        clk_ref             => clk_ref,         -- out   std_logic
        clk_mst             => clk_mst,         -- out   std_logic
        clk_en_ppu          => clk_en_ppu,      -- out   std_logic
        clk_en_cpu          => clk_en_cpu,      -- out   std_logic
        rst_ref             => rst_ref,         -- out   std_logic
        rst_mst             => rst_mst,         -- out   std_logic
        rst_en_ppu          => rst_en_ppu,      -- out   std_logic
        rst_en_cpu          => rst_en_cpu       -- out   std_logic
    );

    -- EEPROM
    eeprom_i0: entity work.at28c256
    generic map (
        DEBUG_PAGE_DUMP_ENABLED     => true,
        BRAM_RD_LATENCY             => x"3"
    )
    port map (
        ps_clk          => ext_rom_clk,         -- in    std_logic;
        ps_rst          => ext_rom_rst,         -- in    std_logic;
        ps_en           => ext_rom_en,          -- in    std_logic;
        ps_we           => ext_rom_we,          -- in    std_logic_vector(3 downto 0);
        ps_addr         => ext_rom_addr,        -- in    std_logic_vector(12 downto 0);
        ps_wr_data      => ext_rom_wr_data,     -- in    std_logic_vector(31 downto 0);
        ps_rd_data      => ext_rom_rd_data,     -- out   std_logic_vector(31 downto 0);
        sbc_clk         => clk_mst,             -- in    std_logic;
        sbc_addr        => (others=>'0'),       -- in    std_logic_vector(14 downto 0);
        sbc_ceb         => '1',                 -- in    std_logic;
        sbc_oeb         => '1',                 -- in    std_logic;
        sbc_web         => '1',                 -- in    std_logic;
        sbc_rd_data     => open                 -- out   std_logic_vector(7 downto 0)
    );

    -- SRAM
    sram_i0: entity work.at28c256
    generic map (
        DEBUG_PAGE_DUMP_ENABLED     => false,
        BRAM_RD_LATENCY             => x"3"
    )
    port map (
        ps_clk          => ext_sram_clk,        -- in    std_logic;
        ps_rst          => ext_sram_rst,        -- in    std_logic;
        ps_en           => ext_sram_en,         -- in    std_logic;
        ps_we           => ext_sram_we,         -- in    std_logic_vector(3 downto 0);
        ps_addr         => ext_sram_addr,       -- in    std_logic_vector(12 downto 0);
        ps_wr_data      => ext_sram_wr_data,    -- in    std_logic_vector(31 downto 0);
        ps_rd_data      => ext_sram_rd_data,    -- out   std_logic_vector(31 downto 0);
        sbc_clk         => clk_mst,              -- in    std_logic;
        sbc_addr        => (others=>'0'),       -- in    std_logic_vector(14 downto 0);
        sbc_ceb         => '1',                 -- in    std_logic;
        sbc_oeb         => '1',                 -- in    std_logic;
        sbc_web         => '1',                 -- in    std_logic;
        sbc_rd_data     => open                 -- out   std_logic_vector(7 downto 0)
    );

end architecture structural;

