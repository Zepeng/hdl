/**
 * @file step1_test_example.c
 * @brief AD4134 Step 1 Test - Configuration Only (No Data Capture)
 *
 * This is a simplified test program for Step 1 that:
 * - Initializes ADC via SPI
 * - Configures for continuous conversion
 * - Validates configuration by reading registers
 * - Does NOT capture data (no DMA, no offload)
 * - ADC outputs data continuously - observe with ILA
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "parameters.h"
#include "ad713x.h"
#include "no_os_spi.h"
#include "no_os_delay.h"
#include "no_os_error.h"
#include "spi_engine.h"
#include "xilinx_gpio.h"
#include "xil_printf.h"

/* Step 1: DMA is REMOVED, don't define this */
// #define AD4134_DMA_BASEADDR  // NOT AVAILABLE IN STEP 1

/* SPI Engine base address - still available for configuration */
#define SPI_ENGINE_BASEADDR  0x44A00000

/* Clock and ODR configuration */
#define CLK_GEN_BASEADDR     0x44B10000
#define ODR_GEN_BASEADDR     0x44B00000

/**
 * @brief Print AD4134 register values for validation
 */
static void print_ad4134_registers(struct ad713x_dev *dev)
{
    uint32_t reg_val;
    int ret;

    xil_printf("\n=== AD4134 Register Dump ===\n");

    /* Chip Type - should be 0x40 */
    ret = ad713x_spi_reg_read(dev, AD713X_REG_CHIP_TYPE, &reg_val);
    if (ret == 0) {
        xil_printf("CHIP_TYPE (0x03):    0x%02X (expected: 0x40)\n", reg_val);
    }

    /* Product ID - should be 0x30 for AD4134 */
    ret = ad713x_spi_reg_read(dev, AD713X_REG_PRODUCT_ID_L, &reg_val);
    if (ret == 0) {
        xil_printf("PRODUCT_ID_L (0x04): 0x%02X (expected: 0x30)\n", reg_val);
    }

    ret = ad713x_spi_reg_read(dev, AD713X_REG_PRODUCT_ID_H, &reg_val);
    if (ret == 0) {
        xil_printf("PRODUCT_ID_H (0x05): 0x%02X\n", reg_val);
    }

    /* Chip Grade */
    ret = ad713x_spi_reg_read(dev, AD713X_REG_CHIP_GRADE, &reg_val);
    if (ret == 0) {
        xil_printf("CHIP_GRADE (0x06):   0x%02X\n", reg_val);
    }

    /* Device Mode */
    ret = ad713x_spi_reg_read(dev, AD713X_REG_DEVICE_CONFIG, &reg_val);
    if (ret == 0) {
        xil_printf("DEVICE_CONFIG (0x14): 0x%02X\n", reg_val);
    }

    /* Channel Enable */
    ret = ad713x_spi_reg_read(dev, AD713X_REG_CHAN_EN, &reg_val);
    if (ret == 0) {
        xil_printf("CHAN_EN (0x16):      0x%02X\n", reg_val);
    }

    /* ODR Selection */
    ret = ad713x_spi_reg_read(dev, AD713X_REG_ODR_VAL_INT_LSB, &reg_val);
    if (ret == 0) {
        xil_printf("ODR_VAL_INT_LSB (0x20): 0x%02X\n", reg_val);
    }

    /* Status Register */
    ret = ad713x_spi_reg_read(dev, AD713X_REG_DEVICE_STATUS, &reg_val);
    if (ret == 0) {
        xil_printf("DEVICE_STATUS (0x00): 0x%02X\n", reg_val);
    }

    xil_printf("=============================\n\n");
}

/**
 * @brief Configure AD4134 for continuous conversion (data output on DOUT pins)
 */
static int configure_ad4134_continuous(struct ad713x_dev *dev)
{
    int ret;

    xil_printf("Configuring AD4134 for continuous conversion...\n");

    /* Set operating mode to continuous conversion */
    ret = ad713x_spi_write_mask(dev,
                                 AD713X_REG_DEVICE_CONFIG,
                                 AD713X_DEVICE_CONFIG_MODE_MSK,
                                 AD713X_DEVICE_CONFIG_MODE(AD713X_CONTINUOUS_MODE));
    if (ret) {
        xil_printf("ERROR: Failed to set continuous mode\n");
        return ret;
    }

    /* Enable all 4 channels */
    ret = ad713x_spi_reg_write(dev, AD713X_REG_CHAN_EN, 0x0F); // Enable CH0-CH3
    if (ret) {
        xil_printf("ERROR: Failed to enable channels\n");
        return ret;
    }

    /* Set ODR to 31.25 kSPS (or your desired rate) */
    ret = ad713x_spi_write_mask(dev,
                                 AD713X_REG_ODR_VAL_INT_LSB,
                                 AD713X_ODR_VAL_INT_LSB_MSK,
                                 AD713X_ODR_VAL_INT_LSB(0x00));
    if (ret) {
        xil_printf("ERROR: Failed to set ODR\n");
        return ret;
    }

    /* Configure output data format (if needed) */
    ret = ad713x_spi_write_mask(dev,
                                 AD713X_REG_DATA_FORMAT,
                                 AD713X_DATA_FORMAT_MODE_MSK,
                                 AD713X_DATA_FORMAT_MODE(AD713X_24_BIT_DATA));
    if (ret) {
        xil_printf("ERROR: Failed to set data format\n");
        return ret;
    }

    xil_printf("Configuration complete!\n");
    xil_printf("ADC is now converting continuously.\n");
    xil_printf("Data is being output on DOUT[3:0] pins.\n");
    xil_printf("Use ILA to observe DCLK, ODR, and DOUT signals.\n\n");

    return 0;
}

/**
 * @brief Main test function for Step 1
 */
int main(void)
{
    struct ad713x_dev *ad4134_dev;
    struct ad713x_init_param ad4134_init;
    struct spi_engine_init_param spi_eng_init_param;
    struct no_os_spi_init_param spi_init;
    int ret;

    xil_printf("\n========================================\n");
    xil_printf("AD4134 Step 1 Test - Configuration Only\n");
    xil_printf("========================================\n\n");

    /* Initialize SPI Engine for configuration */
    spi_eng_init_param.type = SPI_ENGINE;
    spi_eng_init_param.spi_engine_baseaddr = SPI_ENGINE_BASEADDR;
    spi_eng_init_param.cs_delay = 0;
    spi_eng_init_param.data_width = 32;
    spi_eng_init_param.ref_clk_hz = 100000000; // 100 MHz

    spi_init.device_id = 0;
    spi_init.max_speed_hz = 10000000; // 10 MHz SPI
    spi_init.chip_select = 0;
    spi_init.mode = NO_OS_SPI_MODE_3;
    spi_init.platform_ops = &spi_eng_platform_ops;
    spi_init.extra = &spi_eng_init_param;

    /* Initialize AD4134 device structure */
    ad4134_init.spi_init = spi_init;
    ad4134_init.adc_mode = AD713X_CONTINUOUS_MODE;
    ad4134_init.crc_header = AD713X_CRC_16;
    ad4134_init.format = AD713X_24_BIT_DATA;
    ad4134_init.clk_delay_en = false;

    /* Channel configuration */
    for (int i = 0; i < 4; i++) {
        ad4134_init.pwr_mode[i] = AD713X_LOW_POWER;
        ad4134_init.gain[i] = AD713X_GAIN_1;
    }

    /* Initialize the device */
    xil_printf("Initializing AD4134...\n");
    ret = ad713x_init(&ad4134_dev, &ad4134_init);
    if (ret) {
        xil_printf("ERROR: AD4134 initialization failed! (ret=%d)\n", ret);
        return ret;
    }
    xil_printf("AD4134 initialized successfully!\n\n");

    /* Print initial register values */
    print_ad4134_registers(ad4134_dev);

    /* Configure for continuous conversion */
    ret = configure_ad4134_continuous(ad4134_dev);
    if (ret) {
        xil_printf("ERROR: Configuration failed!\n");
        ad713x_remove(ad4134_dev);
        return ret;
    }

    /* Print register values after configuration */
    xil_printf("Register values after configuration:\n");
    print_ad4134_registers(ad4134_dev);

    /* Monitor status periodically */
    xil_printf("\n========================================\n");
    xil_printf("Monitoring ADC Status\n");
    xil_printf("(Press Ctrl+C or reset to stop)\n");
    xil_printf("========================================\n\n");

    uint32_t loop_count = 0;
    while (1) {
        uint32_t status;

        ret = ad713x_spi_reg_read(ad4134_dev, AD713X_REG_DEVICE_STATUS, &status);
        if (ret == 0) {
            xil_printf("Loop %lu: Status = 0x%02X", loop_count, status);

            /* Decode status bits */
            if (status & 0x80) xil_printf(" [ERROR]");
            if (status & 0x40) xil_printf(" [BUSY]");
            if (status & 0x01) xil_printf(" [READY]");
            xil_printf("\n");
        } else {
            xil_printf("Loop %lu: Failed to read status\n", loop_count);
        }

        loop_count++;
        no_os_mdelay(1000); // Read every 1 second
    }

    /* Cleanup (never reached in this example) */
    ad713x_remove(ad4134_dev);

    return 0;
}
