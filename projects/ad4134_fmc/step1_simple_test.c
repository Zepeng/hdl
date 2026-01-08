/***************************************************************************//**
 * @file step1_simple_test.c
 * @brief AD4134 Step 1 - Configuration Only Test (No DMA, No Offload)
 * @author Step 1 Validation Test
 ********************************************************************************
 * Copyright 2025(c) Stanford Readout Project
 *
 * Based on CN0561 example structure
 * This program validates ADC configuration without data capture
 *******************************************************************************/

#include <stdio.h>
#include <sleep.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <xil_cache.h>
#include <xparameters.h>
#include "xil_printf.h"
#include "spi_engine.h"
#include "ad713x.h"
#include "no_os_spi.h"
#include "xilinx_spi.h"
#include "no_os_delay.h"
#include "no_os_gpio.h"
#include "xilinx_gpio.h"
#include "no_os_util.h"
#include "no_os_error.h"

/******************************************************************************/
/********************** Macros and Constants Definitions **********************/
/******************************************************************************/

/* Hardware addresses from xparameters.h */
#define AD4134_SPI_ENGINE_BASEADDR	XPAR_SPI_AD4134_SPI_AD4134_AXI_REGMAP_BASEADDR
#define AD4134_SPI_CS			0
#define SPI_ENG_REF_CLK_FREQ_HZ		100000000
#define GPIO_DEVICE_ID			XPAR_PS7_GPIO_0_DEVICE_ID
#define GPIO_OFFSET			0

/* GPIO definitions (matching your parameters.h) */
#define GPIO_RESETN			(GPIO_OFFSET + 32)
#define GPIO_PDN			(GPIO_OFFSET + 33)
#define GPIO_MODE			(GPIO_OFFSET + 34)
#define GPIO_PINBSPI			(GPIO_OFFSET + 35)

/* Step 1: DMA is REMOVED - comment out this line */
// #define AD4134_DMA_BASEADDR		XPAR_AXI_AD4134_DMA_BASEADDR

/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

/**
 * @brief Print register dump for validation
 */
static void print_ad4134_registers(struct ad713x_dev *dev)
{
	uint32_t reg_val;
	int32_t ret;

	xil_printf("\n=== AD4134 Register Dump ===\n");

	/* Chip Type - should be 0x40 for AD4134 */
	ret = ad713x_spi_reg_read(dev, AD713X_REG_CHIP_TYPE, &reg_val);
	if (ret == 0) {
		xil_printf("CHIP_TYPE (0x03):      0x%02lX ", reg_val);
		if (reg_val == 0x40)
			xil_printf("[OK]\n");
		else
			xil_printf("[ERROR - Expected 0x40]\n");
	} else {
		xil_printf("ERROR reading CHIP_TYPE\n");
	}

	/* Product ID Low */
	ret = ad713x_spi_reg_read(dev, AD713X_REG_PRODUCT_ID_L, &reg_val);
	if (ret == 0) {
		xil_printf("PRODUCT_ID_L (0x04):   0x%02lX\n", reg_val);
	}

	/* Product ID High */
	ret = ad713x_spi_reg_read(dev, AD713X_REG_PRODUCT_ID_H, &reg_val);
	if (ret == 0) {
		xil_printf("PRODUCT_ID_H (0x05):   0x%02lX\n", reg_val);
	}

	/* Chip Grade */
	ret = ad713x_spi_reg_read(dev, AD713X_REG_CHIP_GRADE, &reg_val);
	if (ret == 0) {
		xil_printf("CHIP_GRADE (0x06):     0x%02lX\n", reg_val);
	}

	/* Device Config */
	ret = ad713x_spi_reg_read(dev, AD713X_REG_DEVICE_CONFIG, &reg_val);
	if (ret == 0) {
		xil_printf("DEVICE_CONFIG (0x14):  0x%02lX\n", reg_val);
	}

	/* Channel Enable */
	ret = ad713x_spi_reg_read(dev, AD713X_REG_CHAN_EN, &reg_val);
	if (ret == 0) {
		xil_printf("CHAN_EN (0x16):        0x%02lX\n", reg_val);
	}

	/* Device Status */
	ret = ad713x_spi_reg_read(dev, AD713X_REG_DEVICE_STATUS, &reg_val);
	if (ret == 0) {
		xil_printf("DEVICE_STATUS (0x00):  0x%02lX\n", reg_val);
	}

	xil_printf("============================\n\n");
}

/**
 * @brief Main function
 */
int main()
{
	struct ad713x_dev *ad4134_dev;
	struct ad713x_init_param ad4134_init_param;
	struct no_os_spi_desc *spi_eng_desc;
	struct spi_engine_init_param spi_eng_init_param;
	struct no_os_spi_init_param spi_eng_init_prm;
	struct xil_gpio_init_param gpio_extra_param;
	struct no_os_gpio_init_param ad4134_pdn;
	struct no_os_gpio_init_param ad4134_mode;
	struct no_os_gpio_init_param ad4134_resetn;
	int32_t ret;
	uint32_t loop_count = 0;

	xil_printf("\n\n");
	xil_printf("========================================\n");
	xil_printf("AD4134 Step 1 Test - Configuration Only\n");
	xil_printf("========================================\n");
	xil_printf("DMA: REMOVED (not available)\n");
	xil_printf("Offload: DISABLED (no trigger)\n");
	xil_printf("ILA: Use Vivado Hardware Manager\n");
	xil_printf("========================================\n\n");

	/* Disable cache to avoid DMA issues (even though we have no DMA) */
	Xil_DCacheDisable();
	Xil_ICacheDisable();

	/* Initialize SPI Engine for configuration */
	spi_eng_init_param.type = SPI_ENGINE;
	spi_eng_init_param.spi_engine_baseaddr = AD4134_SPI_ENGINE_BASEADDR;
	spi_eng_init_param.cs_delay = 0;
	spi_eng_init_param.data_width = 32;
	spi_eng_init_param.ref_clk_hz = SPI_ENG_REF_CLK_FREQ_HZ;

	spi_eng_init_prm.chip_select = AD4134_SPI_CS;
	spi_eng_init_prm.max_speed_hz = 10000000; // 10 MHz SPI clock
	spi_eng_init_prm.mode = NO_OS_SPI_MODE_1;
	spi_eng_init_prm.platform_ops = &spi_eng_platform_ops;
	spi_eng_init_prm.extra = (void*)&spi_eng_init_param;

	xil_printf("Initializing SPI engine...\n");
	ret = no_os_spi_init(&spi_eng_desc, &spi_eng_init_prm);
	if (ret) {
		xil_printf("ERROR: SPI engine init failed! (ret=%ld)\n", ret);
		return ret;
	}
	xil_printf("SPI engine initialized successfully\n");

	/* Initialize GPIO for control signals */
	ad4134_pdn.number = GPIO_PDN;
	ad4134_pdn.platform_ops = &xil_gpio_ops;
	ad4134_pdn.extra = &gpio_extra_param;

	ad4134_mode.number = GPIO_MODE;
	ad4134_mode.platform_ops = &xil_gpio_ops;
	ad4134_mode.extra = &gpio_extra_param;

	ad4134_resetn.number = GPIO_RESETN;
	ad4134_resetn.platform_ops = &xil_gpio_ops;
	ad4134_resetn.extra = &gpio_extra_param;

	/* Initialize AD4134 device structure */
	ad4134_init_param.spi_init = spi_eng_init_prm;
	ad4134_init_param.adc_mode = AD713X_CONTINUOUS_MODE;
	ad4134_init_param.crc_header = AD713X_CRC_DISABLE;  // Disable CRC for simplicity
	ad4134_init_param.format = AD713X_24_BIT_DATA;
	ad4134_init_param.clk_delay_en = false;
	ad4134_init_param.pnd = ad4134_pdn;
	ad4134_init_param.mode = ad4134_mode;
	ad4134_init_param.resetn = ad4134_resetn;

	/* Set all channels to low power mode, gain 1 */
	for (int i = 0; i < 4; i++) {
		ad4134_init_param.pwr_mode[i] = AD713X_LOW_POWER;
		ad4134_init_param.gain[i] = AD713X_GAIN_1;
	}

	xil_printf("\nInitializing AD4134 device...\n");
	ret = ad713x_init(&ad4134_dev, &ad4134_init_param);
	if (ret) {
		xil_printf("ERROR: AD4134 initialization failed! (ret=%ld)\n", ret);
		no_os_spi_remove(spi_eng_desc);
		return ret;
	}
	xil_printf("AD4134 initialized successfully!\n\n");

	/* Print initial register values */
	print_ad4134_registers(ad4134_dev);

	/* Configure for continuous conversion */
	xil_printf("Configuring AD4134 for continuous conversion...\n");

	/* Enable all 4 channels */
	ret = ad713x_spi_reg_write(ad4134_dev, AD713X_REG_CHAN_EN, 0x0F);
	if (ret) {
		xil_printf("ERROR: Failed to enable channels\n");
	} else {
		xil_printf("All 4 channels enabled\n");
	}

	/* Set continuous mode */
	ret = ad713x_spi_write_mask(ad4134_dev,
	                             AD713X_REG_DEVICE_CONFIG,
	                             AD713X_DEVICE_CONFIG_MODE_MSK,
	                             AD713X_DEVICE_CONFIG_MODE(AD713X_CONTINUOUS_MODE));
	if (ret) {
		xil_printf("ERROR: Failed to set continuous mode\n");
	} else {
		xil_printf("Continuous conversion mode set\n");
	}

	xil_printf("\nConfiguration complete!\n");
	xil_printf("ADC is now converting continuously.\n");
	xil_printf("Data is being output on DOUT[3:0] pins.\n");
	xil_printf("Use ILA in Vivado Hardware Manager to observe signals.\n\n");

	/* Print registers after configuration */
	xil_printf("Register values after configuration:\n");
	print_ad4134_registers(ad4134_dev);

	/* Monitor status periodically */
	xil_printf("========================================\n");
	xil_printf("Monitoring ADC Status (infinite loop)\n");
	xil_printf("Press reset to stop\n");
	xil_printf("========================================\n\n");

	while (1) {
		uint32_t status;

		ret = ad713x_spi_reg_read(ad4134_dev, AD713X_REG_DEVICE_STATUS, &status);
		if (ret == 0) {
			xil_printf("Loop %4lu: Status = 0x%02lX", loop_count, status);

			/* Decode status bits */
			if (status & 0x80) xil_printf(" [ERROR]");
			if (status & 0x40) xil_printf(" [BUSY]");
			if (status & 0x01) xil_printf(" [READY]");

			xil_printf("\n");
		} else {
			xil_printf("Loop %4lu: Failed to read status (ret=%ld)\n",
			           loop_count, ret);
		}

		loop_count++;

		/* Read every 2 seconds */
		sleep(2);

		/* Every 10 loops, print full register dump */
		if (loop_count % 10 == 0) {
			xil_printf("\n--- Register dump at loop %lu ---\n", loop_count);
			print_ad4134_registers(ad4134_dev);
		}
	}

	/* Cleanup (never reached) */
	ad713x_remove(ad4134_dev);
	no_os_spi_remove(spi_eng_desc);

	return 0;
}
