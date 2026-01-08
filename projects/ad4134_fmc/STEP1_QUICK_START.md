# AD4134 Step 1 - Quick Start Card

## 🎯 What You Need to Do

### 1. Modify parameters.h (1 line change)
```c
// File: /Users/zepengli/work/StanfordReadout/no-OS/projects/ad4134_fmcz/src/parameters.h
// Line 43 - Add comment:
// #define AD4134_DMA_BASEADDR		XPAR_AXI_AD4134_DMA_BASEADDR
```

### 2. Copy test file
```bash
cp /Users/zepengli/work/StanfordReadout/hdl_fork/projects/ad4134_fmc/step1_simple_test.c \
   /Users/zepengli/work/StanfordReadout/no-OS/projects/ad4134_fmcz/src/
```

### 3. Update src.mk
```bash
# File: /Users/zepengli/work/StanfordReadout/no-OS/projects/ad4134_fmcz/src.mk
# Change to use step1_simple_test.c instead of ad4134_continuous_streaming.c
```

### 4. Build HDL
```bash
cd /Users/zepengli/work/StanfordReadout/hdl_fork/projects/ad4134_fmc/zed
make clean
make
```

### 5. Build no-OS
```bash
cd /Users/zepengli/work/StanfordReadout/no-OS/projects/ad4134_fmcz
make clean
make
```

### 6. Test
```bash
# Program FPGA with: ad4134_fmc_zed.bit
# Run no-OS: ad4134_fmcz.elf
# Open ILA in Vivado Hardware Manager
# Open serial console at 115200 baud
```

## ✅ Success Indicators

**Serial Console:**
```
CHIP_TYPE (0x03): 0x40 [OK]
Status: 0x01 [READY]
```

**ILA:**
- DCLK toggles at ~9.6 MHz
- ODR pulses every 85 cycles
- DOUT[3:0] shows serial data

## 📁 Files Created for You

Based on CN0561 structure:

| File | Description |
|------|-------------|
| `step1_simple_test.c` | Working test program |
| `STEP1_NO_OS_INTEGRATION.md` | Detailed integration guide |
| `STEP1_QUICK_START.md` | This file |
| `README_STEP1.md` | Complete overview |
| `STEP1_MODIFICATIONS.md` | HDL changes |
| `STEP1_SUMMARY.md` | Technical details |

## 🔧 Key Differences from Your Streaming Code

| Your Code | Step 1 Test |
|-----------|-------------|
| Uses `spi_engine_offload_init()` | ❌ No offload |
| Uses `spi_engine_offload_transfer()` | ❌ No transfer |
| Uses `axi_dmac_*()` functions | ❌ No DMA |
| Captures data to buffers | ❌ No capture |
| Uses ad713x_init() ✅ | Uses ad713x_init() ✅ |
| Uses register read/write ✅ | Uses register read/write ✅ |

## 📝 What Step 1 Proves

✅ HDL builds correctly
✅ FPGA programs successfully
✅ SPI configuration works
✅ ADC responds to commands
✅ ADC outputs continuous data (see with ILA)
✅ Clocks and timing are correct

## 🚀 After Step 1 Success

You'll be ready to:
1. Design custom capture module (replace SPI offload)
2. Add DMA back for streaming
3. Achieve true continuous data capture

## 💡 Remember

- ADC **IS** outputting data continuously
- FPGA just isn't **capturing** it yet
- ILA lets you **observe** the data
- This validates everything before Step 3

## 🆘 Quick Troubleshooting

| Problem | Solution |
|---------|----------|
| Build error: DMA undefined | Comment out line 43 in parameters.h |
| CHIP_TYPE returns 0xFF | Check SPI wiring, power |
| ILA shows no DCLK | Check clock generator in HDL |
| DOUT stuck at 0 | Check ADC configuration, enable channels |
| Can't link executable | Wrong .c file in src.mk |

## 📞 More Help?

See detailed documentation:
- Integration: `STEP1_NO_OS_INTEGRATION.md`
- ILA usage: `README_STEP1.md` (ILA section)
- HDL changes: `STEP1_MODIFICATIONS.md`
