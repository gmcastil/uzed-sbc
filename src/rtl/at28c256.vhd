-- Hardware model of a 256K (32K x 8) parallel synchronou EEPROM, similar to the
-- Atmel AT28C256 EEPROM
--
-- This module emulates a 32K x 8 ROM with the expected asynchronous interface that one would find
-- in a processor, but with a secondary interface sufficient to be read and written by a block RAM
-- controller. The intent is that a processor model, such as the 6502 can use it as an EEPROM from
-- one interface, but it can be programmed, written to, purged, etc. from a higher level of abstraction
-- such as software running on a processor via a block RAM controller. 

entity at28c256
    port (

        -- PS BRAM controller interface
        -- This section needs to connect up to a Xilinx block RAM controller

        -- PL SRAM interface
        clk         :   in      std_logic;
        chip_enb    :   in      std_logic;
        output_enb  :   in      std_logic;
        wr_enb      :   in      std_logic;
        addr        :   in      std_logic_vector(14 downto 0);
        rd_data     :   out     std_logic_vector(7 downto 0);
        wr_data     :   in      std_logic_vector(7 downto 0)
    );





