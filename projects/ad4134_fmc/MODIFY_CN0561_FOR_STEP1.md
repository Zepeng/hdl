# Modifying CN0561 Project for AD4134 Step 1 Testing

## Overview

Instead of creating a new project, you can modify the working CN0561 project to test your AD4134 Step 1 HDL. This keeps the working structure and just changes the hardware definitions.

## Location

```bash
cd /Users/zepengli/work/StanfordReadout/no-OS/projects/cn0561
```

## Changes Required

### 1. Update parameters.h

File: `src/parameters.h`

```c
/***************************************************************************//**
 *   @file   parameters.h
 *   @brief  Parameters Definitions for AD4134 Step 1 Test
 *******************************************************************************/
#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

#include <xparameters.h>

/* Step 1: Comment out DMA - not available in modified HDL */
// #define CN0561_DMA_BASEADDR		XPAR_AXI_CN0561_DMA_BASEADDR

/* Change to AD4134 addresses (from your HDL build) */
#define CN0561_SPI_ENGINE_BASEADDR	XPAR_SPI_AD4134_SPI_AD4134_AXI_REGMAP_BASEADDR
#define CN0561_SPI_ENG_REF_CLK_FREQ_HZ	100000000
#define CN0561_SPI_CS			0

/* GPIO definitions (match your AD4134 HDL) */
#define SPI_DEVICE_ID			XPAR_PS7_SPI_0_DEVICE_ID
#define GPIO_DEVICE_ID			XPAR_PS7_GPIO_0_DEVICE_ID
#define GPIO_OFFSET			54
#define GPIO_RESETN			GPIO_OFFSET + 32
#define GPIO_PDN			GPIO_OFFSET + 33
#define GPIO_MODE			GPIO_OFFSET + 34
#define GPIO_PINBSPI			GPIO_OFFSET + 35
#define GPIO_0				GPIO_OFFSET + 36
#define GPIO_1				GPIO_OFFSET + 37
#define GPIO_2				GPIO_OFFSET + 38
#define GPIO_4				GPIO_OFFSET + 39
#define GPIO_5				GPIO_OFFSET + 40
#define GPIO_6				GPIO_OFFSET + 41
#define GPIO_7				GPIO_OFFSET + 42
#define CN0561_FMC_CH_NO		4
#define CN0561_FMC_SAMPLE_NO		256

/* Don't need large buffer for Step 1 (no DMA) */
#define ADC_BUFFER_SIZE			1000

/* Enable ZED carrier for GPIO control */
#define CN0561_ZED_CARRIER

/* Data clock frequencies */
#define CORA_Z7S_DATA_CLK_FREQ_HZ   24000000
#define ZED_DATA_CLK_FREQ_HZ        50000000

#ifdef IIO_SUPPORT
#define UART_BAUDRATE			115200
#define UART_DEVICE_ID			XPAR_XUARTPS_0_DEVICE_ID
#define UART_IRQ_ID				XPAR_XUARTPS_0_INTR
#define INTC_DEVICE_ID			XPAR_SCUGIC_SINGLE_DEVICE_ID
#endif // IIO_SUPPORT

#endif /* __PARAMETERS_H__ */
```

### 2. Modify cn0561.c - Remove DMA/Offload Code

File: `src/cn0561.c`

Add Step 1 configuration at the top:

```c
/******************************************************************************/
/********************** Macros and Constants Definitions **********************/
/******************************************************************************/

/* Step 1 Configuration - No DMA, No Offload */
#define STEP1_CONFIG_ONLY  1  // Set to 0 for full streaming (Step 3)
```

Then wrap DMA/offload code:

```c
int main()
{
	struct axi_clkgen *clkgen_cn0561;
	struct axi_clkgen_init clkgen_cn0561_init = {
		.base = XPAR_AXI_CN0561_CLKGEN_BASEADDR,  // Update to AD4134 address
		.name = "ad4134_clkgen",
		.parent_rate = 100000000
	};
	struct ad713x_dev *cn0561_dev;
	struct ad713x_init_param cn0561_init_param;
	int32_t ret;
	uint32_t max_speed_hz = ZED_DATA_CLK_FREQ_HZ;

#if !STEP1_CONFIG_ONLY
	// DMA variables - only for Step 3
	static uint32_t adc_buffer[ADC_BUFFER_SIZE] __attribute__((aligned(1024)));
	uint32_t spi_eng_dma_flg = DMA_LAST | DMA_PARTIAL_REPORTING_EN;
	struct spi_engine_offload_init_param spi_engine_offload_init_param;
	struct spi_engine_offload_message spi_engine_offload_message;
	uint32_t spi_eng_msg_cmds[1];
#endif

	// GPIO and SPI setup (keep as-is)
	struct xil_gpio_init_param gpio_extra_param;
	struct no_os_gpio_init_param cn0561_pnd = {
		.number = GPIO_PDN,
		.platform_ops = &xil_gpio_ops,
		.extra = &gpio_extra_param
	};
	struct no_os_gpio_init_param cn0561_mode = {
		.number = GPIO_MODE,
		.platform_ops = &xil_gpio_ops,
		.extra = &gpio_extra_param
	};
	struct no_os_gpio_init_param cn0561_resetn = {
		.number = GPIO_RESETN,
		.platform_ops = &xil_gpio_ops,
		.extra = &gpio_extra_param
	};

	struct no_os_spi_desc *spi_eng_desc;
	struct spi_engine_init_param spi_eng_init_param  = {
		.type = SPI_ENGINE,
		.spi_engine_baseaddr = CN0561_SPI_ENGINE_BASEADDR,
		.cs_delay = 0,
		.data_width = 32,
		.ref_clk_hz = CN0561_SPI_ENG_REF_CLK_FREQ_HZ
	};
	const struct no_os_spi_init_param spi_eng_init_prm  = {
		.chip_select = CN0561_SPI_CS,
		.max_speed_hz = 10000000,  // 10 MHz for config
		.mode = NO_OS_SPI_MODE_1,
		.platform_ops = &spi_eng_platform_ops,
		.extra = (void*)&spi_eng_init_param,
	};

	// PWM for ODR (keep as-is)
	struct no_os_pwm_desc *axi_pwm;
	struct axi_pwm_init_param axi_zed_pwm_init_odr = {
		.base_addr = XPAR_ODR_GENERATOR_BASEADDR,
		.ref_clock_Hz = 100000000,
		.channel = 1
	};
	struct no_os_pwm_init_param axi_pwm_init_odr = {
		.period_ns = 1000,
		.duty_cycle_ns = 130,
		.phase_ns = 0,
		.platform_ops = &axi_pwm_ops,
		.extra = &axi_zed_pwm_init_odr
	};

	gpio_extra_param.device_id = GPIO_DEVICE_ID;
	gpio_extra_param.type = GPIO_PS;

	// Initialize cache
	Xil_ICacheEnable();
	Xil_DCacheEnable();

	xil_printf("\n========================================\n");
#if STEP1_CONFIG_ONLY
	xil_printf("AD4134 Step 1 - Configuration Test\n");
	xil_printf("DMA: DISABLED\n");
	xil_printf("Offload: DISABLED\n");
#else
	xil_printf("CN0561 Full Streaming Mode\n");
#endif
	xil_printf("========================================\n\n");

	// Initialize clock generator
	ret = axi_clkgen_init(&clkgen_cn0561, &clkgen_cn0561_init);
	if (ret != 0) {
		xil_printf("ERROR: Clock generator init failed\n");
		return -1;
	}

	ret = axi_clkgen_set_rate(clkgen_cn0561, CN0561_SPI_ENG_REF_CLK_FREQ_HZ);
	if (ret != 0) {
		xil_printf("ERROR: Clock generator set rate failed\n");
		return -1;
	}

	// Initialize ODR PWM
	ret = no_os_pwm_init(&axi_pwm, &axi_pwm_init_odr);
	if (ret != 0) {
		xil_printf("ERROR: PWM init failed\n");
		return ret;
	}

	// Configure device init parameters
	cn0561_init_param.adc_data_len = ADC_24_BIT_DATA;
	cn0561_init_param.clk_delay_en = false;
	cn0561_init_param.crc_header = CRC_DISABLE;  // Simpler for Step 1
	cn0561_init_param.dev_id = ID_AD4134;
	cn0561_init_param.format = QUAD_CH_PO;
	cn0561_init_param.gpio_dclkio = NULL;
	cn0561_init_param.gpio_dclkmode = NULL;
	cn0561_init_param.gpio_pnd = &cn0561_pnd;
	cn0561_init_param.gpio_mode = &cn0561_mode;
	cn0561_init_param.gpio_resetn = &cn0561_resetn;
	cn0561_init_param.mode_master_nslave = false;
	cn0561_init_param.dclkmode_free_ngated = false;
	cn0561_init_param.dclkio_out_nin = false;
	cn0561_init_param.pnd = true;
	cn0561_init_param.spi_init_prm = spi_eng_init_prm;
	cn0561_init_param.spi_common_dev = 0;

	// Initialize AD4134
	xil_printf("Initializing AD4134...\n");
	ret = ad713x_init(&cn0561_dev, &cn0561_init_param);
	if (ret != 0) {
		xil_printf("ERROR: AD4134 init failed (ret=%d)\n", ret);
		return -1;
	}
	xil_printf("AD4134 initialized successfully!\n\n");

	// Configure channels
	for (uint32_t adc_channel = CH0; adc_channel <= CH3; adc_channel++) {
		ret = ad713x_dig_filter_sel_ch(cn0561_dev, SINC3, adc_channel);
		if (ret != 0) {
			xil_printf("ERROR: Failed to configure channel %d\n", adc_channel);
			return -1;
		}
	}

	// Print register dump
	uint32_t chip_type, status;
	ad713x_spi_reg_read(cn0561_dev, AD713X_REG_CHIP_TYPE, &chip_type);
	ad713x_spi_reg_read(cn0561_dev, AD713X_REG_DEVICE_STATUS, &status);

	xil_printf("=== AD4134 Register Status ===\n");
	xil_printf("CHIP_TYPE: 0x%02X\n", chip_type);
	xil_printf("STATUS:    0x%02X\n", status);
	xil_printf("==============================\n\n");

#if STEP1_CONFIG_ONLY
	/******************************************************************
	 * STEP 1: Configuration Only Mode
	 * - No DMA offload
	 * - Monitor status periodically
	 * - Use ILA to observe signals
	 ******************************************************************/

	xil_printf("ADC configured for continuous conversion\n");
	xil_printf("Data is output on DOUT pins but not captured\n");
	xil_printf("Use ILA in Vivado to observe signals\n\n");
	xil_printf("Monitoring status (Ctrl+C to stop):\n");

	uint32_t loop_count = 0;
	while (1) {
		ret = ad713x_spi_reg_read(cn0561_dev, AD713X_REG_DEVICE_STATUS, &status);
		if (ret == 0) {
			xil_printf("Loop %4d: Status = 0x%02X", loop_count, status);
			if (status & 0x01) xil_printf(" [READY]");
			if (status & 0x40) xil_printf(" [BUSY]");
			if (status & 0x80) xil_printf(" [ERROR]");
			xil_printf("\n");
		}

		loop_count++;
		sleep(2);  // Every 2 seconds
	}

#else
	/******************************************************************
	 * STEP 3: Full Streaming Mode (DMA + Offload)
	 * - This section for later use
	 ******************************************************************/

	spi_eng_msg_cmds[0] = READ(4);

	ret = no_os_spi_init(&spi_eng_desc, &spi_eng_init_prm);
	if (ret != 0) {
		xil_printf("ERROR: SPI engine init failed\n");
		return -1;
	}

	spi_engine_offload_init_param.rx_dma_baseaddr = CN0561_DMA_BASEADDR;
	spi_engine_offload_init_param.offload_config = OFFLOAD_RX_EN;
	spi_engine_offload_init_param.dma_flags = spi_eng_dma_flg;

	ret = spi_engine_offload_init(spi_eng_desc, &spi_engine_offload_init_param);
	if (ret != 0) {
		xil_printf("ERROR: Offload init failed\n");
		return -1;
	}

	spi_engine_offload_message.commands = spi_eng_msg_cmds;
	spi_engine_offload_message.no_commands = NO_OS_ARRAY_SIZE(spi_eng_msg_cmds);
	spi_engine_offload_message.commands_data = NULL;
	spi_engine_offload_message.rx_addr = (uint32_t)adc_buffer;
	spi_engine_offload_message.tx_addr = 0xA000000;

	// Full streaming code here (your existing CN0561 code)
	xil_printf("Starting streaming...\n");
	// ... rest of original CN0561 streaming code ...

#endif

	// Cleanup
	ad713x_remove(cn0561_dev);

	return 0;
}
```

## Step-by-Step Procedure

### 1. Backup Original Files

```bash
cd /Users/zepengli/work/StanfordReadout/no-OS/projects/cn0561/src
cp parameters.h parameters.h.original
cp cn0561.c cn0561.c.original
```

### 2. Apply Changes

Edit the files as shown above, or:

```bash
# Copy pre-modified versions if you create them
# (see below for automated script)
```

### 3. Build

```bash
cd /Users/zepengli/work/StanfordReadout/no-OS/projects/cn0561
make clean
make
```

### 4. Test

1. Program FPGA with AD4134 Step 1 bitstream
2. Run modified CN0561 application
3. Observe serial console output
4. Use ILA to see signals

## Quick Verification Checklist

Before building:
- [ ] DMA_BASEADDR commented out in parameters.h
- [ ] SPI_ENGINE_BASEADDR updated to AD4134 address
- [ ] GPIO definitions match AD4134 (GPIO_OFFSET = 54)
- [ ] STEP1_CONFIG_ONLY = 1 in cn0561.c
- [ ] CN0561_ZED_CARRIER defined

## Expected Output

```
========================================
AD4134 Step 1 - Configuration Test
DMA: DISABLED
Offload: DISABLED
========================================

Initializing AD4134...
AD4134 initialized successfully!

=== AD4134 Register Status ===
CHIP_TYPE: 0x40
STATUS:    0x01
==============================

ADC configured for continuous conversion
Data is output on DOUT pins but not captured
Use ILA in Vivado to observe signals

Monitoring status (Ctrl+C to stop):
Loop    0: Status = 0x01 [READY]
Loop    1: Status = 0x01 [READY]
Loop    2: Status = 0x01 [READY]
...
```

## Switching Between Step 1 and Step 3

To switch back to full streaming (after Step 3):

```c
// In cn0561.c, change:
#define STEP1_CONFIG_ONLY  0  // Enable full streaming

// In parameters.h, uncomment:
#define CN0561_DMA_BASEADDR		XPAR_AXI_CN0561_DMA_BASEADDR
```

Then rebuild.

## Advantages of This Approach

✅ Uses proven working code structure (CN0561)
✅ Minimal changes required
✅ Easy to switch between Step 1 and Step 3
✅ No new project setup needed
✅ Same build system and Makefile

## Automated Setup Script (Optional)

Create `setup_step1.sh`:

```bash
#!/bin/bash
# Setup CN0561 for AD4134 Step 1 testing

PROJECT_DIR="/Users/zepengli/work/StanfordReadout/no-OS/projects/cn0561"
HDL_DIR="/Users/zepengli/work/StanfordReadout/hdl_fork/projects/ad4134_fmc"

cd $PROJECT_DIR

# Backup originals
cp src/parameters.h src/parameters.h.original
cp src/cn0561.c src/cn0561.c.original

# Apply patches
echo "Modifying parameters.h..."
sed -i.bak 's/^#define CN0561_DMA_BASEADDR/\/\/ #define CN0561_DMA_BASEADDR/' src/parameters.h
sed -i.bak 's/CN0561_SPI_ENGINE_BASEADDR/AD4134_SPI_ENGINE_BASEADDR/g' src/parameters.h

echo "Adding STEP1_CONFIG_ONLY to cn0561.c..."
# Add #define at top of file
# ... (sed commands to modify cn0561.c)

echo "Done! Ready to build."
echo "Run: make clean && make"
```

## Summary

This approach:
- Reuses working CN0561 code structure
- Requires minimal changes (parameters.h and conditional compilation)
- Easy to test and debug
- Smooth transition to Step 3 later

The key is setting `STEP1_CONFIG_ONLY = 1` to skip DMA/offload code while keeping all the configuration and GPIO control that works.
