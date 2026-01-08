# AD4134 Step 1 - Quick Start Guide

## What is Step 1?

**Goal:** Validate that ADC configuration and signal generation work correctly **before** implementing custom data capture.

**What works:**
- ✅ SPI configuration of ADC registers
- ✅ ADC continuous conversion and data output
- ✅ DCLK and ODR signal generation
- ✅ ILA signal observation

**What doesn't work (intentionally):**
- ❌ Data capture to memory
- ❌ DMA streaming
- ❌ `spi_engine_offload_*` functions

## Quick Answer to Your Question

### "Will data be continuous after ADC initialization?"

**ADC Side:** YES - ADC outputs data continuously on DOUT pins after you configure it for continuous mode.

**FPGA Capture:** NO - The FPGA does NOT capture this data in Step 1 (no trigger, no DMA).

**Validation:** Use ILA to observe that data IS being output continuously.

**Why this matters:** Proves your ADC hardware and configuration are working before you build complex capture logic.

## File Guide

### HDL Files (Already Modified)
1. **`common/ad4134_bd.tcl`** - Block design with DMA removed, ILA added
2. **`zed/system_top.v`** - Top level with ILA probe connections

### Documentation Files (New)
1. **`STEP1_SUMMARY.md`** - Technical overview of changes
2. **`STEP1_MODIFICATIONS.md`** - Detailed HDL documentation
3. **`STEP1_NO_OS_GUIDE.md`** - Software modification guide
4. **`step1_test_example.c`** - Example test program
5. **`README_STEP1.md`** - This file

## Build & Test Procedure

### 1. Build HDL (5-30 minutes)

```bash
cd projects/ad4134_fmc/zed
make clean
make
```

**Expected output:** Bitstream `ad4134_fmc_zed.bit` with ILA core

### 2. Modify no-OS Software

**Option A: Use provided example (easiest)**
```bash
# Copy step1_test_example.c to your no-OS project
cp step1_test_example.c /path/to/no-OS/projects/ad4134_fmcz/src/main.c
```

**Option B: Modify your existing code**
- Remove all `spi_engine_offload_*` calls
- Remove all `axi_dmac_*` calls
- Keep only SPI configuration
- See `STEP1_NO_OS_GUIDE.md` for details

### 3. Program and Run

```bash
# Program FPGA with new bitstream
# Run no-OS application
# Open Vivado Hardware Manager for ILA
```

### 4. Validate Success

**Console should show:**
```
CHIP_TYPE:    0x40 ✓
PRODUCT_ID:   0x30 ✓
Status:       0x01 [READY] ✓
```

**ILA should show:**
- DCLK: Toggling at ~9.6 MHz ✓
- ODR: Pulses every 85 DCLK cycles ✓
- DOUT[3:0]: Active serial data ✓

## Understanding the System

### Hardware Signal Flow

```
┌─────────────────────────────────────────────────────────┐
│                     Configuration Path                   │
│  (This works in Step 1)                                  │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  PS7 (ARM) ──SPI──> SPI Engine ──SPI──> ADC Registers   │
│                                                          │
│  You can read/write any register ✓                      │
│                                                          │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                     Clock & Control Path                 │
│  (This works in Step 1)                                  │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  CLK Gen ──9.6MHz──> DCLK ──> ADC (clock input) ✓       │
│                                                          │
│  ODR Gen ──pulses──> ODR ──> ADC (sample timing) ✓      │
│                                                          │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                     Data Output Path                     │
│  (ADC outputs, FPGA observes but doesn't capture)       │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ADC ──DOUT[3:0]──> FPGA ──> SPI Engine (idle)          │
│              │                      │                    │
│              └──────> ILA ──> You can observe ✓         │
│                                                          │
│  SPI Engine Offload: Present but NO TRIGGER ✗           │
│  DMA: Not present ✗                                      │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

### Why Offload Doesn't Work

The SPI Engine offload uses a trigger-based state machine:

```
     ┌─────────────────────────────────────┐
     │  SPI Engine Offload State Machine   │
     ├─────────────────────────────────────┤
     │                                     │
     │  IDLE ──trigger──> ACTIVE           │
     │   ↑                  │              │
     │   │                  │              │
     │   └──────done────────┘              │
     │                                     │
     └─────────────────────────────────────┘

Step 1:  NO TRIGGER ──> Stays in IDLE ──> No capture
Step 3:  TRIGGER connected ──> Cycles through states ──> Capture
```

In Step 1, we **intentionally disconnected the trigger** to prevent automatic capture while we validate the basic setup.

## Memory Map Changes

| Address | Component | Step 1 Status | Step 3 Status |
|---------|-----------|---------------|---------------|
| 0x44A00000 | SPI Engine | ✅ Available | ✅ Available |
| 0x44A30000 | DMA | ❌ REMOVED | ✅ Will restore |
| 0x44B00000 | ODR Generator | ✅ Available | ✅ Available |
| 0x44B10000 | Clock Gen | ✅ Available | ✅ Available |

**Important:** Do NOT access 0x44A30000 in Step 1 - it will cause a bus error!

## Typical Issues & Solutions

### Issue: Build fails with ILA errors
**Solution:** Ensure you have Xilinx ILA IP core licensed/available

### Issue: "CHIP_TYPE returns 0xFF"
**Solution:**
- Check SPI wiring
- Check ADC power
- Verify SPI clock speed (should be ≤ 10 MHz)

### Issue: ILA shows no DCLK activity
**Solution:**
- Check clock generator settings
- Verify system clock is present
- Check constraint file for DCLK pin

### Issue: ILA shows DOUT stuck at 0
**Solution:**
- Verify ADC is in continuous mode (register 0x14)
- Check that channels are enabled (register 0x16)
- Verify ODR is configured correctly

### Issue: Software crashes when accessing DMA
**Solution:**
- Remove ALL references to `spi_engine_offload_*`
- Remove ALL references to `axi_dmac_*`
- Use simple test program first

## Next Steps After Success

Once Step 1 validates correctly:

### Step 2: Design Custom Capture Module

**Create new HDL module:**
```verilog
module ad4134_custom_capture (
    input wire         clk,
    input wire         reset_n,

    // From ADC
    input wire         dclk,
    input wire         odr,
    input wire [3:0]   dout,

    // AXI-Stream output
    output wire [127:0] m_axis_tdata,
    output wire         m_axis_tvalid,
    input wire          m_axis_tready
);
    // Your deserializer logic here
    // Convert 4 serial streams to parallel samples
    // Output via AXI-Stream
endmodule
```

### Step 3: Integrate with DMA

**Modify `ad4134_bd.tcl`:**
```tcl
# Remove ILA (or keep for debug)
# Add custom capture module
# Re-add axi_ad4134_dma
# Connect: custom_capture → DMA → Memory
```

**Modify no-OS:**
```c
// Initialize custom capture module
custom_capture_init();

// Re-add DMA operations
dma_init();
dma_start();

// Your existing streaming buffer logic can stay!
```

## Resources

- **AD4134 Datasheet:** Check register descriptions
- **SPI Engine Documentation:** Understanding offload mechanism
- **ILA User Guide:** Vivado debugging

## Summary Checklist

**Before moving to Step 3, verify:**
- [ ] HDL builds without errors
- [ ] no-OS compiles and runs
- [ ] CHIP_TYPE register reads correctly (0x40)
- [ ] ILA shows DCLK toggling
- [ ] ILA shows ODR pulses at correct rate
- [ ] ILA shows DOUT data activity on all 4 channels
- [ ] Timing relationships look correct
- [ ] No SPI communication errors

**If all checked, you're ready for Step 3!**

---

For detailed information, see:
- Technical details: `STEP1_SUMMARY.md`
- HDL changes: `STEP1_MODIFICATIONS.md`
- Software guide: `STEP1_NO_OS_GUIDE.md`
