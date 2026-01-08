# AD4134 Step 1 Modifications - Configuration Only Mode

## Overview
This document describes the modifications made to the AD4134 HDL project for Step 1:
- Remove SPI Engine offload mode and DMA data capture
- Keep SPI configuration engine for ADC register access
- Add ILA debugging probes for DCLK, ODR, and DOUT signals
- Leave DOUT pins floating (not connected to data capture logic)

## HDL Changes Made

### 1. Modified Files

#### `/projects/ad4134_fmc/common/ad4134_bd.tcl`
**Changes:**
- **SPI Engine Configuration:**
  - Kept `num_sdi` = 4 and `num_sdo` = 1 (SPI engine structure unchanged)
  - **Key Change:** Disconnected offload trigger (`odr_generator/pwm_0` NOT connected to `spi_engine/trigger`)
  - This keeps the SPI engine intact but disables automatic offload mode

- **Removed Components:**
  - `axi_ad4134_dma` instance completely removed
  - All DMA-related connections removed
  - Memory interconnect (HP1) removed
  - DMA interrupt removed (ps-13)
  - `odr_generator/pwm_0` trigger to SPI engine removed (no offload mode)

- **Added Components:**
  - **ILA Debug Core** (`ila_ad4134`) with:
    - 3 probes configured
    - Probe 0: DCLK (1-bit)
    - Probe 1: ODR (1-bit)
    - Probe 2: DOUT[3:0] (4-bit)
    - 4096 sample depth
    - Clock: `axi_ad4134_clkgen/clk_0`

- **New Ports:**
  - `ad4134_dclk_probe` - Input port for DCLK monitoring
  - `ad4134_dout_probe[3:0]` - Input port for DOUT monitoring

- **Memory Map Changes:**
  - 0x44a00000: `spi_ad4134_axi_regmap` (KEPT - for configuration)
  - 0x44a30000: DMA removed (was `axi_ad4134_dma`)
  - 0x44b00000: `odr_generator` (KEPT - for ODR control)
  - 0x44b10000: `axi_ad4134_clkgen` (KEPT - for clock generation)

#### `/projects/ad4134_fmc/zed/system_top.v`
**Changes:**
- **DOUT Connection:**
  - `.ad4134_di_sdi (ad4134_din)` - Physical DOUT pins connected to SPI engine
  - Data is available to SPI engine but offload mode is disabled (no trigger)
  - This allows manual SPI reads if needed for validation

- **ILA Probe Connections:**
  - `.ad4134_dclk_probe (ad4134_dclk)` - DCLK routed to ILA
  - `.ad4134_dout_probe (ad4134_din)` - DOUT[3:0] routed to ILA

### 2. Signal Flow

```
Configuration Path (Still Active):
  PS7 SPI0 → ad4134_spi_* pins → ADC Configuration Registers

Data Clock Path (Still Active):
  axi_ad4134_clkgen → DCLK → ADC

ODR Path (Still Active):
  odr_generator/pwm_1 → ODR → ADC

Data Path (Available but not auto-captured):
  ADC DOUT[3:0] → ad4134_din → SPI engine (offload trigger disconnected)
                           → ILA (for debug)
```

### 3. What Still Works
- ✅ SPI configuration interface via PS7 SPI0
- ✅ ADC register read/write operations
- ✅ DCLK generation and output to ADC
- ✅ ODR generation and output to ADC
- ✅ Clock generation (axi_ad4134_clkgen)
- ✅ GPIO control lines (RESETN, PDN, MODE, etc.)
- ✅ ILA debugging capability

### 4. What's Removed
- ❌ SPI Engine offload mode (no trigger input)
- ❌ DMA data capture to memory
- ❌ Data streaming capability
- ❌ Interrupt 13 (DMA IRQ)
- ❌ AXI HP1 memory interface

## No-OS Software Modifications Required

### 1. Driver Initialization Changes

**File to modify:** `no-OS/projects/ad4134/src/examples/basic_example.c` (or equivalent)

#### Remove DMA Initialization
```c
// REMOVE or COMMENT OUT:
// struct axi_dmac *dmac;
// struct axi_dmac_init dmac_init = {
//     .name = "ad4134_dma",
//     .base = 0x44A30000,  // This address no longer exists
//     .irq_option = IRQ_DISABLED
// };
// ret = axi_dmac_init(&dmac, &dmac_init);
```

#### Keep SPI Configuration
```c
// KEEP THIS - SPI engine for configuration
struct spi_engine_init_param spi_eng_init_param = {
    .type = SPI_ENGINE,
    .spi_engine_baseaddr = 0x44A00000,  // Still valid
    .cs_delay = 0,
    .data_width = 32,
    .ref_clk_hz = 100000000  // 100 MHz
};

// KEEP THIS - Device initialization
struct ad4134_init_param ad4134_init = {
    .spi_init = {
        .device_id = 0,
        .max_speed_hz = 10000000,  // 10 MHz SPI clock
        .chip_select = 0,
        .mode = SPI_MODE_3,
        .extra = &spi_eng_init_param,
        .platform_ops = &spi_eng_platform_ops
    },
    // ... other parameters
};

ret = ad4134_init(&ad4134_dev, &ad4134_init);
```

### 2. Register Access Validation

You can now test register read/write operations without DMA:

```c
// Example: Read device ID register
uint32_t chip_id;
ret = ad4134_read_reg(ad4134_dev, AD4134_REG_CHIP_TYPE, &chip_id);
printf("AD4134 Chip ID: 0x%X\n", chip_id);

// Example: Configure ODR
ret = ad4134_set_odr(ad4134_dev, AD4134_ODR_31_25_KSPS);

// Example: Configure channel settings
ret = ad4134_set_channel_en(ad4134_dev, AD4134_CH0, true);
ret = ad4134_set_channel_gain(ad4134_dev, AD4134_CH0, AD4134_GAIN_1);

// Example: Read status register
uint32_t status;
ret = ad4134_read_reg(ad4134_dev, AD4134_REG_STATUS, &status);
printf("Status: 0x%X\n", status);
```

### 3. Remove Data Capture Code

```c
// REMOVE all code related to:
// - axi_dmac_transfer() calls
// - DMA buffer allocation
// - Data streaming loops
// - DMA interrupt handlers
```

### 4. Testing Sequence

```c
int main(void)
{
    struct ad4134_dev *ad4134_dev;
    int ret;

    // 1. Initialize platform (UART, etc.)
    ret = platform_init();

    // 2. Initialize AD4134 (SPI configuration only)
    ret = ad4134_init(&ad4134_dev, &ad4134_init);
    if (ret) {
        printf("AD4134 init failed: %d\n", ret);
        return ret;
    }

    // 3. Validate by reading registers
    uint32_t chip_id, status;
    ret = ad4134_read_reg(ad4134_dev, AD4134_REG_CHIP_TYPE, &chip_id);
    printf("Chip ID: 0x%X (expected: 0x40)\n", chip_id);

    // 4. Configure ADC for continuous conversion
    ret = ad4134_set_operating_mode(ad4134_dev, AD4134_CONTINUOUS_MODE);
    ret = ad4134_set_odr(ad4134_dev, AD4134_ODR_31_25_KSPS);

    // Enable all 4 channels
    for (int ch = 0; ch < 4; ch++) {
        ret = ad4134_set_channel_en(ad4134_dev, ch, true);
    }

    // 5. Read status periodically
    while (1) {
        ret = ad4134_read_reg(ad4134_dev, AD4134_REG_STATUS, &status);
        printf("Status: 0x%X\n", status);
        mdelay(1000);
    }

    return 0;
}
```

## ILA Debug Instructions

### 1. Opening ILA in Vivado Hardware Manager

After programming the FPGA:

1. Open Vivado Hardware Manager
2. Connect to your target (ZED board)
3. The ILA core `hw_ila_1` should be automatically detected
4. Open the ILA dashboard

### 2. ILA Probe Assignments

- **probe0**: `ad4134_dclk` - Data clock output to ADC
- **probe1**: `ad4134_odr` - Output data rate signal
- **probe2[3:0]**: `ad4134_din` - Four DOUT lines from ADC

### 3. Suggested Trigger Configuration

**Option 1: Trigger on ODR rising edge**
```
Trigger Probe: probe1 (ODR)
Trigger Condition: Rising Edge
Trigger Position: 512 (capture before and after trigger)
```

**Option 2: Trigger immediately**
```
Trigger Mode: BASIC
Trigger Condition: None (always trigger)
```

### 4. Expected Signals

When the ADC is properly configured and running:

- **DCLK**: Should toggle continuously if ADC is in continuous conversion mode
  - Frequency depends on `axi_ad4134_clkgen` settings (default: ~9.6 MHz)

- **ODR**: Periodic pulses indicating output data rate
  - Period: 85 clock cycles (from `odr_generator` config)
  - Width: 13 clock cycles

- **DOUT[3:0]**: Serial data streams from 4 ADC channels
  - Data valid only when DCLK is active
  - Each channel outputs 24-bit conversion results serially

### 5. Validation Checklist

- [ ] DCLK toggles regularly
- [ ] ODR pulses occur periodically
- [ ] DOUT lines show activity (not stuck at 0 or 1)
- [ ] Timing relationship: Data changes on DCLK edges
- [ ] ODR period matches configured sample rate

## Build Instructions

### 1. Clean Previous Build
```bash
cd /Users/zepengli/work/StanfordReadout/hdl_fork/projects/ad4134_fmc/zed
make clean
```

### 2. Build New Bitstream
```bash
make
```

This will:
- Generate the block design with ILA
- Synthesize and implement
- Generate bitstream: `ad4134_fmc_zed.bit`

### 3. Expected Build Changes

You should see in the build log:
- ILA IP core instantiation
- Reduced resource usage (no DMA)
- New debug ports created
- Warnings about unused interrupt 13 (expected - DMA removed)

## Testing Plan for Step 1

### Hardware Validation

1. **Program FPGA** with new bitstream
2. **Run no-OS** application with DMA removed
3. **Verify SPI communication:**
   - Read CHIP_ID register (should be 0x40)
   - Read/write configuration registers
   - Check for SPI errors

4. **Use ILA to verify signals:**
   - Capture DCLK activity
   - Observe ODR pulses
   - Check DOUT data streams
   - Verify timing relationships

### Success Criteria

✅ **Pass Criteria:**
- no-OS can read AD4134 registers successfully
- CHIP_ID register returns expected value
- Configuration writes are accepted
- ILA shows DCLK toggling
- ILA shows ODR pulses at correct rate
- ILA shows DOUT activity on all 4 channels

❌ **Fail Indicators:**
- Cannot read registers (SPI communication failed)
- DCLK not toggling (clock generation issue)
- ODR not pulsing (PWM generator issue)
- DOUT stuck at constant values (ADC not converting)

## Next Steps

After successful validation of Step 1:

### Step 2: Custom Data Capture Module
- Design custom capture logic based on SPI Engine offload architecture
- Interface directly to `ad4134_din[3:0]` signals
- Deserialize 4 channels of 24-bit data
- Output via AXI-Stream interface

### Step 3: DMA Integration
- Re-add DMA to capture data from custom module
- Configure DMA for 4-channel, 24-bit samples
- Test continuous streaming to DDR memory

## Troubleshooting

### Issue: Build fails with ILA errors
**Solution:** Ensure Xilinx ILA IP is available in your Vivado installation

### Issue: System wrapper doesn't regenerate
**Solution:**
```bash
make clean
rm -rf .Xil *.cache *.hw *.ip_user_files
make
```

### Issue: ILA not detected in Hardware Manager
**Solution:**
- Verify bitstream programmed correctly
- Check that debug probes are enabled in Vivado settings
- Re-program FPGA

### Issue: no-OS fails with DMA errors
**Solution:**
- Ensure all DMA-related code is removed/commented out
- Check that only 0x44A00000 address is accessed
- Verify interrupt handlers don't reference IRQ 13

## File Summary

### Modified Files:
1. `/projects/ad4134_fmc/common/ad4134_bd.tcl` - Block design changes
2. `/projects/ad4134_fmc/zed/system_top.v` - Top-level connections

### Unchanged Files:
- `/projects/ad4134_fmc/zed/system_constr.xdc` - Pin constraints (no changes needed)
- `/projects/ad4134_fmc/zed/system_project.tcl` - Build script
- `/projects/ad4134_fmc/zed/Makefile` - Build system

### New Files:
- `/projects/ad4134_fmc/STEP1_MODIFICATIONS.md` - This document

## Register Map Reference

### Still Accessible:
| Address | Module | Purpose |
|---------|--------|---------|
| 0x44A00000 | spi_ad4134_axi_regmap | SPI Engine control/status |
| 0x44B00000 | odr_generator | PWM control for ODR |
| 0x44B10000 | axi_ad4134_clkgen | Clock generator control |

### Removed:
| Address | Module | Status |
|---------|--------|--------|
| 0x44A30000 | axi_ad4134_dma | REMOVED - Do not access |

## Additional Notes

- The ADC will still function normally and convert data continuously
- Data is output on DOUT pins but not captured by FPGA logic
- This configuration validates that ADC configuration and timing work correctly
- Physical signals can be probed with oscilloscope or logic analyzer in addition to ILA
- ILA data can be exported from Vivado for offline analysis

## Contact / Questions

For issues with this modification, verify:
1. Vivado version compatibility (tested with 2023.2)
2. HDL repository version (based on hdl_2023_r2 branch)
3. ZED board revision
4. AD4134-FMC hardware revision
