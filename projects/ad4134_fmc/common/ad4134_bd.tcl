###############################################################################
## Copyright (C) 2023-2024 Analog Devices, Inc. All rights reserved.
### SPDX short identifier: ADIBSD
###############################################################################

create_bd_intf_port -mode Master -vlnv analog.com:interface:spi_engine_rtl:1.0 ad4134_di
create_bd_port -dir O ad4134_odr

# Create ports for ILA probing
create_bd_port -dir I ad4134_dclk_probe
create_bd_port -dir I -from 3 -to 0 ad4134_dout_probe

# create a SPI Engine architecture for ADC (Configuration only, no offload)

source $ad_hdl_dir/library/spi_engine/scripts/spi_engine.tcl

set data_width    32
set async_spi_clk 1
set num_cs        1
set num_sdi       0
set num_sdo       0
set sdi_delay     0
set echo_sclk     0

set hier_spi_engine spi_ad4134

spi_engine_create $hier_spi_engine $data_width $async_spi_clk $num_cs $num_sdi $num_sdo $sdi_delay $echo_sclk

# clkgen

ad_ip_instance axi_clkgen axi_ad4134_clkgen
ad_ip_parameter axi_ad4134_clkgen CONFIG.VCO_DIV 5
ad_ip_parameter axi_ad4134_clkgen CONFIG.VCO_MUL 48
ad_ip_parameter axi_ad4134_clkgen CONFIG.CLK0_DIV 10

# DMA removed - we will use custom data capture module in later steps
# For now, DOUT pins will be monitored via ILA only

# ILA for debugging - probe DCLK, ODR, and 4 DOUT signals
ad_ip_instance ila ila_ad4134
ad_ip_parameter ila_ad4134 CONFIG.C_NUM_OF_PROBES 3
ad_ip_parameter ila_ad4134 CONFIG.C_PROBE0_WIDTH 1
ad_ip_parameter ila_ad4134 CONFIG.C_PROBE1_WIDTH 1
ad_ip_parameter ila_ad4134 CONFIG.C_PROBE2_WIDTH 4
ad_ip_parameter ila_ad4134 CONFIG.C_DATA_DEPTH 4096
ad_ip_parameter ila_ad4134 CONFIG.C_EN_STRG_QUAL 1
ad_ip_parameter ila_ad4134 CONFIG.ALL_PROBE_SAME_MU true

# odr generator

ad_ip_instance axi_pwm_gen odr_generator
ad_ip_parameter odr_generator CONFIG.N_PWMS 2
ad_ip_parameter odr_generator CONFIG.PULSE_0_PERIOD 85
ad_ip_parameter odr_generator CONFIG.PULSE_0_WIDTH 1
ad_ip_parameter odr_generator CONFIG.PULSE_0_OFFSET 3
ad_ip_parameter odr_generator CONFIG.PULSE_1_PERIOD 85
ad_ip_parameter odr_generator CONFIG.PULSE_1_WIDTH 13

ad_connect odr_generator/ext_clk axi_ad4134_clkgen/clk_0
ad_connect odr_generator/pwm_1 ad4134_odr
# Note: pwm_0 trigger removed since no offload mode

ad_connect  axi_ad4134_clkgen/clk_0 $hier_spi_engine/spi_clk
ad_connect  $sys_cpu_clk axi_ad4134_clkgen/clk
ad_connect  $sys_cpu_clk $hier_spi_engine/clk
ad_connect  sys_cpu_resetn $hier_spi_engine/resetn

ad_connect  $hier_spi_engine/m_spi ad4134_di

# ILA connections - probe DCLK (probe0), ODR (probe1), and DOUT[3:0] (probe2)
ad_connect  axi_ad4134_clkgen/clk_0 ila_ad4134/clk
ad_connect  ad4134_dclk_probe ila_ad4134/probe0
ad_connect  ad4134_odr ila_ad4134/probe1
ad_connect  ad4134_dout_probe ila_ad4134/probe2

# AXI address definitions

ad_cpu_interconnect 0x44a00000 $hier_spi_engine/${hier_spi_engine}_axi_regmap
# 0x44a30000 axi_ad4134_dma - REMOVED (no DMA in this configuration)
ad_cpu_interconnect 0x44b00000 odr_generator
ad_cpu_interconnect 0x44b10000 axi_ad4134_clkgen

# interrupts

# ad_cpu_interrupt "ps-13" "mb-13" axi_ad4134_dma/irq - REMOVED (no DMA)
ad_cpu_interrupt "ps-12" "mb-12" $hier_spi_engine/irq

# memory interconnects - REMOVED (no DMA to memory)
# ad_mem_hp1_interconnect $sys_cpu_clk sys_ps7/S_AXI_HP1
# ad_mem_hp1_interconnect $sys_cpu_clk axi_ad4134_dma/m_dest_axi
