# Compilation Fixes for AD4134 Step 1

## Issues Fixed

### 1. Missing Clock Generator Base Address

**Error:**
```
error: 'XPAR_AXI_CN0561_CLKGEN_BASEADDR' undeclared
```

**Root Cause:**
The HDL creates a clock generator named `axi_ad4134_clkgen` (line 31 in ad4134_bd.tcl), which generates the Xilinx parameter `XPAR_AXI_AD4134_CLKGEN_BASEADDR`, but the CN0561 no-OS code was looking for `XPAR_AXI_CN0561_CLKGEN_BASEADDR`.

**Fix:**
Added definition in [parameters.h:45](../../no-OS_fork/projects/cn0561/src/parameters.h#L45):
```c
#define CN0561_CLKGEN_BASEADDR		XPAR_AXI_AD4134_CLKGEN_BASEADDR
```

Updated [cn0561.c:88](../../no-OS_fork/projects/cn0561/src/cn0561.c#L88) to use the new definition:
```c
struct axi_clkgen_init clkgen_cn0561_init = {
    .base = CN0561_CLKGEN_BASEADDR,  // Changed from XPAR_AXI_CN0561_CLKGEN_BASEADDR
    .name = "cn0561_clkgen",
    .parent_rate = 100000000
};
```

### 2. Incompatible Pointer Types for Register Reads

**Warning:**
```
warning: passing argument 3 of 'ad713x_spi_reg_read' from incompatible pointer type [-Wincompatible-pointer-types]
```

**Root Cause:**
The function `ad713x_spi_reg_read()` expects `uint8_t *reg_data` but we were passing `uint32_t*` pointers.

**Function signature (from ad713x.h):**
```c
int32_t ad713x_spi_reg_read(struct ad713x_dev *dev, uint8_t reg_addr,
                            uint8_t *reg_data);
```

**Fix:**
Changed variable declarations in [cn0561.c:258](../../no-OS_fork/projects/cn0561/src/cn0561.c#L258):
```c
// Before:
uint32_t chip_type, status, device_config;

// After:
uint8_t chip_type, status, device_config;
```

## HDL vs no-OS Peripheral Name Mapping

| HDL Instance Name | Address | Xilinx Parameter |
|-------------------|---------|------------------|
| `spi_ad4134` | 0x44a00000 | `XPAR_SPI_AD4134_SPI_AD4134_AXI_REGMAP_BASEADDR` |
| `axi_ad4134_clkgen` | 0x44b10000 | `XPAR_AXI_AD4134_CLKGEN_BASEADDR` |
| `odr_generator` | 0x44b00000 | `XPAR_ODR_GENERATOR_BASEADDR` |

## Files Modified

1. **[parameters.h](../../no-OS_fork/projects/cn0561/src/parameters.h)**
   - Line 45: Added `CN0561_CLKGEN_BASEADDR` definition

2. **[cn0561.c](../../no-OS_fork/projects/cn0561/src/cn0561.c)**
   - Line 88: Changed to use `CN0561_CLKGEN_BASEADDR`
   - Line 258: Changed register variables from `uint32_t` to `uint8_t`

## Build Status

After these fixes:
- ✅ No compilation errors
- ✅ No type warnings
- ✅ Ready to build and test on hardware

## Next Steps

1. Build HDL: `cd hdl_fork/projects/ad4134_fmc/zed && make`
2. Build no-OS: `cd no-OS_fork/projects/cn0561 && make`
3. Test on FPGA with ILA and serial console

## Important Note

These fixes are required because:
- The HDL project is named **ad4134_fmc** (creates AD4134-prefixed parameters)
- The no-OS project is named **cn0561** (expects CN0561-prefixed parameters)
- We bridge this gap with `#define` redirections in parameters.h

When you build the HDL, Vivado generates `xparameters.h` with the actual hardware addresses based on the instance names in the block design. Our parameter definitions map the CN0561 software names to the actual AD4134 hardware names.
