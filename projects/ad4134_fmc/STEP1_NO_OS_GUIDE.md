# Step 1 no-OS Software Guide

## Understanding the Data Flow

### Question: Will data be continuous after ADC initialization?

**Short Answer:**
- ✅ **ADC outputs data continuously** on DOUT pins (observable with ILA)
- ❌ **FPGA does NOT capture data** (no offload trigger, no DMA)

### Detailed Explanation:

```
┌─────────────────────────────────────────────────────────┐
│ ADC Side (Continuous)                                   │
├─────────────────────────────────────────────────────────┤
│  1. You configure ADC via SPI                           │
│  2. Set to continuous conversion mode                   │
│  3. ADC continuously converts at configured ODR         │
│  4. ADC outputs data on DOUT[3:0] pins                  │
│  5. DCLK toggles to clock data out                      │
│  6. This happens REGARDLESS of FPGA capture             │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ FPGA Side (NOT Capturing in Step 1)                     │
├─────────────────────────────────────────────────────────┤
│  1. SPI Engine receives data on SDI lines               │
│  2. Offload module exists but has NO TRIGGER            │
│  3. Without trigger, offload FSM stays IDLE             │
│  4. Data passes through but is NOT captured             │
│  5. No DMA to store data                                │
│  6. Your offload functions won't work                   │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ ILA Observation (Step 1 Validation)                     │
├─────────────────────────────────────────────────────────┤
│  - ILA probes DCLK, ODR, DOUT[3:0]                      │
│  - You can SEE data is being output                     │
│  - Validates ADC is working correctly                   │
│  - Validates timing and signal integrity                │
└─────────────────────────────────────────────────────────┘
```

## What to Change in Your Existing Code

### Your Current Code Structure

Looking at your `ad4134_continuous_streaming.c`, it has:

```c
struct streaming_context {
    struct spi_engine_offload_message *offload_msg;  // ← Won't work
    // ... buffers, DMA, etc.
};

// These functions won't work in Step 1:
spi_engine_offload_init(...)       // ← No DMA at 0x44A30000
spi_engine_offload_transfer(...)   // ← No trigger, no DMA
```

### For Step 1: Create Simplified Test

**Option 1: Simple validation test (RECOMMENDED)**

See the example file: `step1_test_example.c`

This test:
1. Initializes ADC via SPI (works)
2. Configures continuous mode (works)
3. Reads registers for validation (works)
4. **Does NOT** try to capture data
5. Relies on ILA to observe signals

**Option 2: Modify your existing streaming code**

If you want to keep your existing file structure:

```c
// In your parameters.h or main file:
#define STEP1_CONFIG_ONLY  // Define this for Step 1 testing

#ifdef STEP1_CONFIG_ONLY
    // Step 1: Configuration only, no data capture

    // Initialize ADC
    ret = ad713x_init(&ad4134_dev, &init_param);

    // Configure for continuous mode
    ad713x_set_continuous_mode(ad4134_dev);

    // Enable channels
    ad713x_enable_channels(ad4134_dev, 0x0F);

    // Print registers
    print_ad4134_registers(ad4134_dev);

    printf("ADC configured. Data is output on DOUT pins.\n");
    printf("Use ILA to observe signals.\n");

    // Monitor status
    while(1) {
        uint32_t status;
        ad713x_read_status(ad4134_dev, &status);
        printf("Status: 0x%X\n", status);
        mdelay(1000);
    }

#else
    // Step 3: Full streaming (after custom capture module added)

    // Your existing offload + DMA code here
    streaming_init(&ctx, &init_param);
    streaming_start(&ctx);
    // ... etc

#endif
```

## Step-by-Step Integration Plan

### Current Situation:
- You have: `ad4134_continuous_streaming.c` with full DMA/offload code
- Step 1 HDL: SPI config only, no DMA, no offload trigger

### Recommended Approach:

#### Phase 1: Use simplified test (Step 1)
```bash
# In your no-OS project:
# Temporarily use step1_test_example.c instead of your streaming code
cp step1_test_example.c src/main.c
make
```

**This validates:**
- ✅ SPI configuration works
- ✅ ADC responds correctly
- ✅ ADC outputs data (see with ILA)
- ✅ Register access is functional

#### Phase 2: Design custom capture (Step 3 prep)
- Design your custom capture HDL module
- Keep it separate from existing code
- Test with simulation if possible

#### Phase 3: Integrate custom capture (Step 3)
```c
// Modify your streaming code to use custom capture instead of SPI offload:

// REMOVE:
// spi_engine_offload_init()
// spi_engine_offload_transfer()

// ADD:
custom_capture_init()     // Your custom module
custom_capture_start()    // Triggers continuous capture
dma_transfer()            // Re-added DMA
```

## API Changes Needed

### Functions That WON'T Work in Step 1:

```c
❌ spi_engine_offload_init()      // No DMA at 0x44A30000
❌ spi_engine_offload_transfer()  // No trigger connected
❌ axi_dmac_*()                   // No DMA IP present
❌ Any DMA buffer operations      // No memory path
```

### Functions That WILL Work in Step 1:

```c
✅ ad713x_init()                  // SPI configuration works
✅ ad713x_spi_reg_read()          // Register read works
✅ ad713x_spi_reg_write()         // Register write works
✅ ad713x_set_*() configuration   // All config functions work
✅ ad713x_enable_channels()       // Channel enable works
✅ ad713x_set_operating_mode()    // Mode setting works
```

## Example Test Sequence

### 1. Compile and Run Step 1 Test

```bash
cd no-OS/projects/ad4134_fmcz
# Modify Makefile to use step1_test_example.c
make
```

### 2. Expected Console Output

```
========================================
AD4134 Step 1 Test - Configuration Only
========================================

Initializing AD4134...
AD4134 initialized successfully!

=== AD4134 Register Dump ===
CHIP_TYPE (0x03):    0x40 (expected: 0x40)
PRODUCT_ID_L (0x04): 0x30 (expected: 0x30)
PRODUCT_ID_H (0x05): 0x41
CHIP_GRADE (0x06):   0x11
DEVICE_CONFIG (0x14): 0x01
CHAN_EN (0x16):      0x0F
ODR_VAL_INT_LSB (0x20): 0x00
DEVICE_STATUS (0x00): 0x01
=============================

Configuring AD4134 for continuous conversion...
Configuration complete!
ADC is now converting continuously.
Data is being output on DOUT[3:0] pins.
Use ILA to observe DCLK, ODR, and DOUT signals.

========================================
Monitoring ADC Status
========================================

Loop 0: Status = 0x01 [READY]
Loop 1: Status = 0x01 [READY]
Loop 2: Status = 0x01 [READY]
...
```

### 3. ILA Observation

In Vivado Hardware Manager:
1. Trigger on ODR rising edge
2. Observe:
   - **DCLK**: Should toggle regularly
   - **ODR**: Periodic pulses
   - **DOUT[3:0]**: Serial data streams

### 4. Success Criteria

✅ **Step 1 is successful if:**
- Console shows correct CHIP_ID (0x40)
- Console shows correct PRODUCT_ID (0x30)
- Status register indicates READY (0x01)
- ILA shows DCLK toggling
- ILA shows ODR pulses
- ILA shows activity on DOUT[3:0]

## Transition to Step 3

After Step 1 success, you'll:

1. **Design custom capture module** (HDL)
   - Replace SPI engine offload
   - Deserialize DOUT[3:0] data
   - Output AXI-Stream

2. **Re-add DMA** (HDL)
   - Connect to custom module
   - Configure for continuous streaming

3. **Update software** (no-OS)
   - Remove `spi_engine_offload_*` calls
   - Add `custom_capture_*` calls
   - Re-add DMA operations
   - Keep streaming buffer logic

## Files Provided

1. **step1_test_example.c** - Simple validation program
2. **STEP1_NO_OS_GUIDE.md** - This document
3. **STEP1_MODIFICATIONS.md** - HDL changes documentation
4. **STEP1_SUMMARY.md** - Quick reference

## Key Takeaway

**Step 1 Purpose:**
- Validate SPI configuration path
- Validate ADC is outputting data
- Prove hardware connectivity
- **NOT** meant for data capture

**Step 3 Goal:**
- Design custom capture that DOES work continuously
- Replace offload with your own logic
- Achieve true continuous streaming
