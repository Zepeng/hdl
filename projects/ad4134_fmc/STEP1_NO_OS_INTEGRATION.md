# Step 1 no-OS Integration Guide

## Quick Integration (Simplest Approach)

Since you already have a working no-OS project structure at `/Users/zepengli/work/StanfordReadout/no-OS/projects/ad4134_fmcz/`, here's how to integrate the Step 1 test:

### Method 1: Replace Main Source File

```bash
# 1. Backup your existing file
cd /Users/zepengli/work/StanfordReadout/no-OS/projects/ad4134_fmcz/src
cp ad4134_continuous_streaming.c ad4134_continuous_streaming.c.backup

# 2. Copy the Step 1 test
cp /Users/zepengli/work/StanfordReadout/hdl_fork/projects/ad4134_fmc/step1_simple_test.c \
   ad4134_continuous_streaming.c

# 3. Update parameters.h - comment out DMA reference
```

### Method 2: Create Separate Test File

Keep your streaming code and add the test as a separate file:

```bash
cd /Users/zepengli/work/StanfordReadout/no-OS/projects/ad4134_fmcz/src

# Copy test file
cp /Users/zepengli/work/StanfordReadout/hdl_fork/projects/ad4134_fmc/step1_simple_test.c .

# Modify src.mk to use it (see below)
```

## Required Changes to parameters.h

Edit `/Users/zepengli/work/StanfordReadout/no-OS/projects/ad4134_fmcz/src/parameters.h`:

```c
// Line 43 - COMMENT OUT THIS LINE (DMA not available in Step 1)
// #define AD4134_DMA_BASEADDR		XPAR_AXI_AD4134_DMA_BASEADDR

// Keep everything else as-is:
#define AD4134_SPI_ENGINE_BASEADDR	XPAR_SPI_AD4134_SPI_AD4134_AXI_REGMAP_BASEADDR
#define AD713x_SPI_ENG_REF_CLK_FREQ_HZ	100000000
#define AD4134_1_SPI_CS			0
// ... GPIO definitions stay the same ...
```

## Modifying src.mk (If using separate file)

Edit `/Users/zepengli/work/StanfordReadout/no-OS/projects/ad4134_fmcz/src.mk`:

### Option A: Use Step 1 test only
```makefile
# Comment out streaming code
# SRC_DIRS += $(PROJECT)/src
# SRCS += $(PROJECT)/src/ad4134_continuous_streaming.c

# Add Step 1 test
SRCS += $(PROJECT)/src/step1_simple_test.c
```

### Option B: Conditional compilation (advanced)
```makefile
# Use environment variable to select
ifndef STEP1_TEST
    SRCS += $(PROJECT)/src/ad4134_continuous_streaming.c
else
    SRCS += $(PROJECT)/src/step1_simple_test.c
endif
```

Then build with:
```bash
make STEP1_TEST=1
```

## Build Process

### 1. Set Environment Variables

```bash
# Make sure these are set
export VIVADO=/tools/Xilinx/Vivado/2023.2
export WORKSPACE=/path/to/your/workspace
```

### 2. Generate xparameters.h

You need `xparameters.h` from your HDL build:

```bash
# After building HDL in Vivado:
cd /Users/zepengli/work/StanfordReadout/hdl_fork/projects/ad4134_fmc/zed
make

# xparameters.h will be at:
# ad4134_fmc_zed.sdk/system_top_hw_platform_0/ps7_cortexa9_0/include/xparameters.h

# Copy to your no-OS build location or update include paths
```

### 3. Build no-OS

```bash
cd /Users/zepengli/work/StanfordReadout/no-OS/projects/ad4134_fmcz
make
```

Expected output:
```
Building project ad4134_fmcz...
Compiling step1_simple_test.c...
Linking...
Build successful!
Output: ad4134_fmcz.elf
```

## Handling Common Compilation Errors

### Error: "XPAR_AXI_AD4134_DMA_BASEADDR undeclared"

**Cause:** DMA is removed from HDL but referenced in code

**Solution:** Comment out in `parameters.h`:
```c
// #define AD4134_DMA_BASEADDR		XPAR_AXI_AD4134_DMA_BASEADDR
```

### Error: "undefined reference to spi_engine_offload_init"

**Cause:** Trying to link offload functions that aren't used

**Solution:** Make sure you're compiling `step1_simple_test.c`, not your streaming code

### Error: "no_os_spi_init failed"

**Cause:** Wrong SPI engine base address or parameters

**Solution:** Verify in `xparameters.h`:
```c
// Should exist:
#define XPAR_SPI_AD4134_SPI_AD4134_AXI_REGMAP_BASEADDR 0x44A00000

// Should NOT exist (DMA removed):
// #define XPAR_AXI_AD4134_DMA_BASEADDR  <- This will be missing
```

### Error: "ad713x_init returned error"

**Possible causes:**
1. HDL not programmed
2. Wrong FPGA bitstream
3. SPI timing issues

**Debug steps:**
```c
// Add debug prints in your code:
xil_printf("SPI baseaddr: 0x%08X\n", AD4134_SPI_ENGINE_BASEADDR);
xil_printf("Attempting SPI init...\n");
ret = no_os_spi_init(...);
xil_printf("SPI init returned: %d\n", ret);
```

## Running the Test

### 1. Program FPGA

In Vivado Hardware Manager:
```
Tools → Open Hardware Manager
Open Target → Auto Connect
Program Device → Select ad4134_fmc_zed.bit
```

### 2. Download and Run ELF

Using Vitis/SDK:
```
Right-click on project → Run As → Launch on Hardware
```

Or using XSCT:
```bash
xsct
connect
targets -set -filter {name =~ "ARM*#0"}
dow ad4134_fmcz.elf
con
```

### 3. Open Serial Console

```bash
# Find your USB serial port
ls /dev/ttyUSB*

# Open with screen
screen /dev/ttyUSB1 115200

# Or minicom
minicom -D /dev/ttyUSB1 -b 115200
```

### 4. Expected Output

```
========================================
AD4134 Step 1 Test - Configuration Only
========================================
DMA: REMOVED (not available)
Offload: DISABLED (no trigger)
ILA: Use Vivado Hardware Manager
========================================

Initializing SPI engine...
SPI engine initialized successfully

Initializing AD4134 device...
AD4134 initialized successfully!

=== AD4134 Register Dump ===
CHIP_TYPE (0x03):      0x40 [OK]
PRODUCT_ID_L (0x04):   0x30
PRODUCT_ID_H (0x05):   0x41
CHIP_GRADE (0x06):     0x11
DEVICE_CONFIG (0x14):  0x01
CHAN_EN (0x16):        0x0F
DEVICE_STATUS (0x00):  0x01
============================

Configuring AD4134 for continuous conversion...
All 4 channels enabled
Continuous conversion mode set

Configuration complete!
ADC is now converting continuously.
Data is being output on DOUT[3:0] pins.
Use ILA in Vivado Hardware Manager to observe signals.

Register values after configuration:
=== AD4134 Register Dump ===
CHIP_TYPE (0x03):      0x40 [OK]
PRODUCT_ID_L (0x04):   0x30
PRODUCT_ID_H (0x05):   0x41
CHIP_GRADE (0x06):     0x11
DEVICE_CONFIG (0x14):  0x01
CHAN_EN (0x16):        0x0F
DEVICE_STATUS (0x00):  0x01
============================

========================================
Monitoring ADC Status (infinite loop)
Press reset to stop
========================================

Loop    0: Status = 0x01 [READY]
Loop    1: Status = 0x01 [READY]
Loop    2: Status = 0x01 [READY]
...
```

## Project File Structure

After integration, your project should look like:

```
no-OS/projects/ad4134_fmcz/
├── Makefile
├── builds.json
├── src/
│   ├── parameters.h                         (MODIFIED - DMA commented out)
│   ├── ad4134_continuous_streaming.c        (BACKUP - for Step 3)
│   ├── step1_simple_test.c                  (NEW - Step 1 test)
│   └── ... (other files)
├── src.mk                                    (MODIFIED - build step1 test)
└── ... (other files)
```

## Comparison with CN0561

Your Step 1 test follows the same pattern as CN0561:

| Feature | CN0561 | AD4134 Step 1 |
|---------|--------|---------------|
| SPI Engine Init | ✅ Yes | ✅ Yes |
| Device Init (ad713x_init) | ✅ Yes | ✅ Yes |
| GPIO Control | ✅ Yes | ✅ Yes |
| Register Access | ✅ Yes | ✅ Yes |
| DMA/Offload | ✅ Used | ❌ Disabled |
| ILA Debug | ❌ No | ✅ Yes |
| Continuous Mode | ✅ Yes | ✅ Yes |

## Troubleshooting Build Issues

### Issue: Missing xparameters.h

```bash
# Generate from HDL build:
cd hdl_fork/projects/ad4134_fmc/zed
make
# Find xparameters.h in build output
# Copy to no-OS include path
```

### Issue: Linker errors about missing functions

Check `src.mk` - make sure only `step1_simple_test.c` is included:
```makefile
SRCS += $(PROJECT)/src/step1_simple_test.c
# NOT:
# SRCS += $(PROJECT)/src/ad4134_continuous_streaming.c
```

### Issue: "no rule to make target"

Clean and rebuild:
```bash
make clean
make
```

## Next Steps

After successful Step 1 validation:

1. **Verify with ILA** - See signals in Vivado Hardware Manager
2. **Check all registers** - Validate configuration is correct
3. **Move to Step 3** - Design custom capture module
4. **Restore streaming code** - Integrate with custom capture

## File Locations Summary

| File | Location | Purpose |
|------|----------|---------|
| step1_simple_test.c | `hdl_fork/projects/ad4134_fmc/` | Step 1 test source (copy to no-OS) |
| parameters.h | `no-OS/projects/ad4134_fmcz/src/` | Hardware definitions (modify) |
| src.mk | `no-OS/projects/ad4134_fmcz/` | Build configuration (modify) |
| xparameters.h | HDL build output | Generated by Vivado (need for build) |
| ad4134_fmcz.elf | no-OS build output | Final executable |

## Success Criteria

✅ Step 1 is successful when:
- Code compiles without errors
- CHIP_TYPE reads as 0x40
- Status shows READY (0x01)
- ILA shows DCLK toggling
- ILA shows ODR pulses
- ILA shows DOUT activity
- No DMA or offload errors

You're now ready to move to Step 3 when all criteria are met!
