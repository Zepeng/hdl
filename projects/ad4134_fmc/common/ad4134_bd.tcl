###############################################################################
## Copyright (C) 2023-2024 Analog Devices, Inc. All rights reserved.
### SPDX short identifier: ADIBSD
###############################################################################

create_bd_intf_port -mode Master -vlnv analog.com:interface:spi_engine_rtl:1.0 ad4134_di
create_bd_port -dir O ad4134_odr
create_bd_port -dir O ad4134_dclk

# Create ports for ILA probing
create_bd_port -dir I ad4134_dclk_probe
create_bd_port -dir I -from 3 -to 0 ad4134_dout_probe

# create a SPI Engine architecture for ADC (Configuration only, no offload)

source $ad_hdl_dir/library/spi_engine/scripts/spi_engine.tcl

set data_width    32
set async_spi_clk 1
set num_cs        1
set num_sdi       4
set num_sdo       1
set sdi_delay     0
set echo_sclk     0

set hier_spi_engine spi_ad4134

spi_engine_create $hier_spi_engine $data_width $async_spi_clk $num_cs $num_sdi $num_sdo $sdi_delay $echo_sclk

# clkgen

ad_ip_instance axi_clkgen axi_ad4134_clkgen
ad_ip_parameter axi_ad4134_clkgen CONFIG.ENABLE_CLKOUT1 true
ad_ip_parameter axi_ad4134_clkgen CONFIG.VCO_DIV 1
ad_ip_parameter axi_ad4134_clkgen CONFIG.VCO_MUL 10
ad_ip_parameter axi_ad4134_clkgen CONFIG.CLK0_DIV 10
ad_ip_parameter axi_ad4134_clkgen CONFIG.CLK1_DIV 20

# DMA for custom AXI-stream capture
ad_ip_instance axi_dmac axi_ad4134_dma
ad_ip_parameter axi_ad4134_dma CONFIG.DMA_TYPE_SRC 1
ad_ip_parameter axi_ad4134_dma CONFIG.DMA_TYPE_DEST 0
ad_ip_parameter axi_ad4134_dma CONFIG.CYCLIC 0
ad_ip_parameter axi_ad4134_dma CONFIG.SYNC_TRANSFER_START 0
ad_ip_parameter axi_ad4134_dma CONFIG.DMA_2D_TRANSFER 0
ad_ip_parameter axi_ad4134_dma CONFIG.DMA_DATA_WIDTH_SRC 128
ad_ip_parameter axi_ad4134_dma CONFIG.DMA_DATA_WIDTH_DEST 64
ad_ip_parameter axi_ad4134_dma CONFIG.MAX_BYTES_PER_BURST 128
ad_ip_parameter axi_ad4134_dma CONFIG.FIFO_SIZE 16

# ILA for debugging - probe DCLK, ODR, and 4 DOUT signals
ad_ip_instance ila ila_ad4134
ad_ip_parameter ila_ad4134 CONFIG.C_NUM_OF_PROBES 3
ad_ip_parameter ila_ad4134 CONFIG.C_MONITOR_TYPE Native
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

# Ensure custom capture module is in the project sources before BD creation.
if {[llength [get_files -quiet *ad4134_axis_capture.v]] == 0} {
  add_files -norecurse "$ad_hdl_dir/projects/ad4134_fmc/common/ad4134_axis_capture.v"
}

# custom capture module (AXI-stream source)
create_bd_cell -type module -reference ad4134_axis_capture ad4134_capture

ad_connect odr_generator/ext_clk axi_ad4134_clkgen/clk_1
ad_connect odr_generator/pwm_1 ad4134_odr
# Note: pwm_0 trigger NOT connected - offload mode disabled for Step 1
# ad_connect odr_generator/pwm_0 $hier_spi_engine/trigger

ad_connect  axi_ad4134_clkgen/clk_1 ad4134_dclk
ad_connect  axi_ad4134_clkgen/clk_1 ad4134_capture/clk
ad_connect  axi_ad4134_clkgen/clk_0 $hier_spi_engine/spi_clk
ad_connect  $sys_cpu_clk axi_ad4134_clkgen/clk
ad_connect  $sys_cpu_clk $hier_spi_engine/clk
ad_connect  sys_cpu_resetn $hier_spi_engine/resetn
ad_connect  sys_cpu_resetn ad4134_capture/resetn
ad_connect  odr_generator/pwm_1 ad4134_capture/odr
ad_connect  ad4134_dout_probe ad4134_capture/din

ad_connect  $hier_spi_engine/m_spi ad4134_di

# ILA connections - probe DCLK (probe0), ODR (probe1), and DOUT[3:0] (probe2)
ad_connect  axi_ad4134_clkgen/clk_0 ila_ad4134/clk
connect_bd_net [get_bd_ports ad4134_dclk_probe] [get_bd_pins ila_ad4134/probe0]
connect_bd_net [get_bd_ports ad4134_odr] [get_bd_pins ila_ad4134/probe1]
connect_bd_net [get_bd_ports ad4134_dout_probe] [get_bd_pins ila_ad4134/probe2]

# AXI-stream capture to DMA
ad_connect  axi_ad4134_clkgen/clk_1 axi_ad4134_dma/s_axis_aclk
ad_connect  ad4134_capture/m_axis_tvalid axi_ad4134_dma/s_axis_valid
ad_connect  ad4134_capture/m_axis_tdata axi_ad4134_dma/s_axis_data
ad_connect  axi_ad4134_dma/s_axis_ready ad4134_capture/m_axis_tready
ad_connect  ad4134_capture/m_axis_tkeep axi_ad4134_dma/s_axis_keep
ad_connect  ad4134_capture/m_axis_tstrb axi_ad4134_dma/s_axis_strb
ad_connect  ad4134_capture/m_axis_tlast axi_ad4134_dma/s_axis_last
ad_connect  ad4134_capture/m_axis_tuser axi_ad4134_dma/s_axis_user
ad_connect  ad4134_capture/m_axis_tid axi_ad4134_dma/s_axis_id
ad_connect  ad4134_capture/m_axis_tdest axi_ad4134_dma/s_axis_dest
ad_connect  $sys_cpu_clk axi_ad4134_dma/s_axi_aclk
ad_connect  sys_cpu_resetn axi_ad4134_dma/s_axi_aresetn
ad_connect  sys_cpu_resetn axi_ad4134_dma/m_dest_axi_aresetn
if {[llength [get_bd_pins -quiet axi_ad4134_dma/sync]] > 0} {
  ad_connect  GND axi_ad4134_dma/sync
}

# AXI address definitions

ad_cpu_interconnect 0x44a00000 $hier_spi_engine/${hier_spi_engine}_axi_regmap
ad_cpu_interconnect 0x44a30000 axi_ad4134_dma
ad_cpu_interconnect 0x44b00000 odr_generator
ad_cpu_interconnect 0x44b10000 axi_ad4134_clkgen

# interrupts

ad_cpu_interrupt "ps-13" "mb-13" axi_ad4134_dma/irq
ad_cpu_interrupt "ps-12" "mb-12" $hier_spi_engine/irq

# memory interconnects
ad_mem_hp1_interconnect $sys_cpu_clk sys_ps7/S_AXI_HP1
ad_mem_hp1_interconnect $sys_cpu_clk axi_ad4134_dma/m_dest_axi
