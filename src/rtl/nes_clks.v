module nes_clks //#(
//)
(
  // This should be connected an external clock input pad or port from the
  // processing subsystem - all clocking resources, including buffers are
  // contained in this module. This is expected to be a 100MHz external clock
  // from either an oscillator or a fabric clock from the processor. From it, we
  // generate a 236.25MHz clock, which is then used to generate the CPU and PPU
  // clocks.
  input   wire      clk_ext,
  // Asynchronous active high reset which essentially just resets the MMCM that
  // is the base clock for all generated clocks.
  input   wire      rst_ext,

  // The master clock, at least for NTSC (2C02) applications is defined as
  // 236.25MHz / 11 which works out to 21.477272MHz +/- 40Hz. The accurace of
  // the clock is not nearly as significan as the fact that the CPU and PPU
  // clocks are a) derived from the same source and b) maintain the same
  // relationship. 
  output  wire      clk_master,
  // CPU clock defined as the master clock divided by 12 or 1.789773 MHz
  output  wire      clk_cpu,
  // PPU clock defined as the master clock divided by 4 or 5.369318 MHz
  output  wire      clk_ppu,
  // MMCM locked indicator which can (and should) be used to synthesize
  // synchronous resets for all generated clocks.
  output  wire      mmcm_locked
);

  localparam  GND   = 1'b0; 
  localparam  VCC   = 1'b1;

  wire    feedback;
  wire    mmcm_locked;

  MMCME2_ADV #(
    .BANDWIDTH              ("OPTIMIZED"),
    .CLKOUT4_CASCADE        ("FALSE"),
    .COMPENSATION           ("ZHOLD"),
    .STARTUP_WAIT           ("FALSE"),
    .DIVCLK_DIVIDE          (5),
    .CLKFBOUT_MULT_F        (47.250),
    .CLKFBOUT_PHASE         (0.000),
    .CLKFBOUT_USE_FINE_PS   ("FALSE"),
    .CLKOUT0_DIVIDE_F       (4.000),
    .CLKOUT0_PHASE          (0.000),
    .CLKOUT0_DUTY_CYCLE     (0.500),
    .CLKOUT0_USE_FINE_PS    ("FALSE"),
    .CLKIN1_PERIOD          (10.000)
  )
  MMCME2_ADV_i0 (
    // Input and feedback clocks. Note that since there is no required phase
    // relationship between the input and output clock, there is no BUFG
    // inserted into the feedback loop.  We simply wire it from the feedback
    // clock output to the input
    .CLKFBIN                (feedback),
    .CLKIN1                 (clk_ext),
    // No secondary input clock is used and we do not switch reference clocks.
    // Note that high indicates CLKIN1 and a low indicates CLKIN2.
    .CLKIN2                 (GND),
    .CLKINSEL               (VCC),

    // Output clocks
    .CLKFBOUT               (feedback),
    .CLKFBOUTB              (),
    .CLKOUT0                (clk_236m25_mmcm),
    .CLKOUT0B               (),
    .CLKOUT1                (),
    .CLKOUT1B               (),
    .CLKOUT2                (),
    .CLKOUT2B               (),
    .CLKOUT3                (),
    .CLKOUT3B               (),
    .CLKOUT4                (),
    .CLKOUT5                (),
    .CLKOUT6                (),

    // Other control and status signals
    .LOCKED                 (mmcm_locked),
    .CLKINSTOPPED           (),
    .CLKFBSTOPPED           (),
    .PWRDWN                 (GND),
    .RST                    (rst_ext),

    // Dynamic reconfiguration port (DRP) interface (unused)
    .DADDR                  (7'h0),
    .DCLK                   (1'b0),
    .DEN                    (1'b0),
    .DI                     (16'h0),
    .DO                     (),
    .DRDY                   (),
    .DWE                    (1'b0),
    // Dynamic phase shift (unused)
    .PSCLK                  (1'b0),
    .PSEN                   (1'b0),
    .PSINCDEC               (1'b0),
    .PSDONE                 ()
  );

endmodule
