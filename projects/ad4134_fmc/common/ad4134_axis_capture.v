// ***************************************************************************
// ***************************************************************************
// Copyright (C) 2024 Analog Devices, Inc. All rights reserved.
//
// Simple AD4134 capture block: shifts 4-bit DOUT data on each clock edge
// following an ODR pulse, then outputs one 128-bit AXI-stream sample.
// ***************************************************************************
// ***************************************************************************

`timescale 1ns/100ps

module ad4134_axis_capture #(
  parameter integer CHANNELS = 4,
  parameter integer DATA_WIDTH = 32,
  parameter integer WORD_LEN = 32
) (
  input                          clk,
  input                          resetn,

  input                          odr,
  input  [CHANNELS-1:0]          din,

  output reg [CHANNELS*DATA_WIDTH-1:0] m_axis_tdata,
  output reg                     m_axis_tvalid,
  input                          m_axis_tready,
  output [CHANNELS*DATA_WIDTH/8-1:0] m_axis_tkeep,
  output [CHANNELS*DATA_WIDTH/8-1:0] m_axis_tstrb,
  output                         m_axis_tlast,
  output [0:0]                   m_axis_tuser,
  output [7:0]                   m_axis_tid,
  output [3:0]                   m_axis_tdest,
  output reg                     overflow
);

  localparam integer COUNT_WIDTH = 6;

  reg [DATA_WIDTH-1:0] shift_data_0 = {DATA_WIDTH{1'b0}};
  reg [DATA_WIDTH-1:0] shift_data_1 = {DATA_WIDTH{1'b0}};
  reg [DATA_WIDTH-1:0] shift_data_2 = {DATA_WIDTH{1'b0}};
  reg [DATA_WIDTH-1:0] shift_data_3 = {DATA_WIDTH{1'b0}};

  reg [COUNT_WIDTH-1:0] bit_count = {COUNT_WIDTH{1'b0}};
  reg capture_active = 1'b0;
  reg odr_d = 1'b0;

  wire odr_rise;
  wire [DATA_WIDTH-1:0] shift_next_0;
  wire [DATA_WIDTH-1:0] shift_next_1;
  wire [DATA_WIDTH-1:0] shift_next_2;
  wire [DATA_WIDTH-1:0] shift_next_3;

  assign odr_rise = odr & ~odr_d;

  assign shift_next_0 = {shift_data_0[DATA_WIDTH-2:0], din[0]};
  assign shift_next_1 = {shift_data_1[DATA_WIDTH-2:0], din[1]};
  assign shift_next_2 = {shift_data_2[DATA_WIDTH-2:0], din[2]};
  assign shift_next_3 = {shift_data_3[DATA_WIDTH-2:0], din[3]};

  assign m_axis_tkeep = {CHANNELS*DATA_WIDTH/8{1'b1}};
  assign m_axis_tstrb = {CHANNELS*DATA_WIDTH/8{1'b1}};
  assign m_axis_tlast = 1'b0;
  assign m_axis_tuser = 1'b0;
  assign m_axis_tid = 8'h00;
  assign m_axis_tdest = 4'h0;

  always @(posedge clk) begin
    if (!resetn) begin
      shift_data_0 <= {DATA_WIDTH{1'b0}};
      shift_data_1 <= {DATA_WIDTH{1'b0}};
      shift_data_2 <= {DATA_WIDTH{1'b0}};
      shift_data_3 <= {DATA_WIDTH{1'b0}};
      bit_count <= {COUNT_WIDTH{1'b0}};
      capture_active <= 1'b0;
      odr_d <= 1'b0;
      m_axis_tdata <= {(CHANNELS*DATA_WIDTH){1'b0}};
      m_axis_tvalid <= 1'b0;
      overflow <= 1'b0;
    end else begin
      odr_d <= odr;

      if (m_axis_tvalid && m_axis_tready) begin
        m_axis_tvalid <= 1'b0;
      end

      if (capture_active) begin
        shift_data_0 <= shift_next_0;
        shift_data_1 <= shift_next_1;
        shift_data_2 <= shift_next_2;
        shift_data_3 <= shift_next_3;

        if (bit_count <= 1) begin
          capture_active <= 1'b0;
          bit_count <= {COUNT_WIDTH{1'b0}};

          if (!m_axis_tvalid) begin
            m_axis_tdata <= {shift_next_3, shift_next_2, shift_next_1, shift_next_0};
            m_axis_tvalid <= 1'b1;
          end else begin
            overflow <= 1'b1;
          end
        end else begin
          bit_count <= bit_count - 1'b1;
        end
      end else if (odr_rise) begin
        shift_data_0 <= {DATA_WIDTH{1'b0}};
        shift_data_1 <= {DATA_WIDTH{1'b0}};
        shift_data_2 <= {DATA_WIDTH{1'b0}};
        shift_data_3 <= {DATA_WIDTH{1'b0}};
        bit_count <= WORD_LEN[COUNT_WIDTH-1:0];
        capture_active <= 1'b1;
      end
    end
  end

endmodule
