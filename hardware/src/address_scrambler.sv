// Copyright 2021 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51

// Description: Scrambles the address in such a way, that part of the memory is accessed
// sequentially and part is interleaved.
// Current constraints:

// Author: Samuel Riedel <sriedel@iis.ee.ethz.ch>

module address_scrambler #(
  parameter int unsigned AddrWidth         = 32,
  parameter int unsigned ByteOffset        = 2,
  parameter int unsigned NumTiles          = 2,
  parameter int unsigned NumBanksPerTile   = 2,
  parameter bit          Bypass            = 0,
  parameter int unsigned SeqMemSizePerTile = 4*1024,
  parameter int unsigned HeapSeqMemSizePerTile = 8*2048,
  parameter int unsigned TCDMSize = 1024*1024
) (
  input  logic [AddrWidth-1:0] address_i,
  input  logic [7:0]           group_factor_i,
  output logic [AddrWidth-1:0] address_o
);
  // Stack Sequential Settings
  localparam int unsigned BankOffsetBits    = $clog2(NumBanksPerTile);
  localparam int unsigned TileIdBits        = $clog2(NumTiles);
  localparam int unsigned SeqPerTileBits    = $clog2(SeqMemSizePerTile);
  localparam int unsigned SeqTotalBits      = SeqPerTileBits+TileIdBits;
  localparam int unsigned ConstantBitsLSB   = ByteOffset + BankOffsetBits;
  localparam int unsigned ScrambleBits      = SeqPerTileBits-ConstantBitsLSB;

  // Heap Sequential Settings
  localparam int unsigned HeapSeqPerTileBits = $clog2(HeapSeqMemSizePerTile);      // log2(8*2048) = 14
  localparam int unsigned HeapSeqTotalBits   = HeapSeqPerTileBits+TileIdBits;      // 14+7=21, used for address_o assignment 
  localparam int unsigned HeapScrambleBits   = HeapSeqPerTileBits-ConstantBitsLSB; // 7 bits default

  if (Bypass || NumTiles < 2) begin
    assign address_o = address_i;
  end else begin
    logic [ScrambleBits-1:0]    scramble;    // Address bits that have to be shuffled around
    logic [TileIdBits-1:0]      tile_id;     // Which tile does  this address region belong to

    // Leave this part of the address unchanged
    // The LSBs that correspond to the offset inside a tile. These are the byte offset (bank width)
    // and the Bank offset (Number of Banks in tile)
    // assign address_o[ConstantBitsLSB-1:0] = address_i[ConstantBitsLSB-1:0];
    // The MSBs that are outside of the sequential memory size. Currently the sequential memory size
    // always starts at 0. These are all the MSBs up to SeqMemSizePerTile*NumTiles
    // assign address_o[AddrWidth-1:SeqTotalBits] = address_i[AddrWidth-1:SeqTotalBits];

    // Scramble the middle part
    // Bits that would have gone to different tiles but now go to increasing lines in the same tile
    assign scramble = address_i[SeqPerTileBits-1:ConstantBitsLSB]; // Bits that would
    // Bits that would have gone to increasing lines in the same tile but now go to different tiles
    assign tile_id  = address_i[SeqTotalBits-1:SeqPerTileBits];

    // ----- Heap Sequential Signals ----- //
    // | ----- |     Row   Index     | Tile ID | Const | 
    // | ----- | - | Pid | heap_scramble | ele | Const | (pre-scramble)
    // | ----- | - | heap_scramble | Pid | ele | Const | (post-scramble)

    // shift_index == log2(group_index) = log2(1)   = 0: Tile level sequential --> | ----- | - |    Pid    | heap_scramble | Const |
    // shift_index == log2(group_index) = log2(128) = 7; Fully interleaved     --> | ----- | - | heap_scramble |    ele    | Const |
    logic [2:0] shift_index;                       // how many bits need to shift covering `ele`
    assign shift_index = (group_factor_i == 128) ? 7 : 
                         (group_factor_i == 64)  ? 6 : 
                         (group_factor_i == 32)  ? 5 : 
                         (group_factor_i == 16)  ? 4 : 
                         (group_factor_i == 8 )  ? 3 : 
                         (group_factor_i == 4 )  ? 2 : 
                         (group_factor_i == 2 )  ? 1 : 0; // TODO

    logic [HeapScrambleBits-1:0]  heap_scramble;   // Row index bits which are shuffled around 
    logic [TileIdBits-1:0]        heap_pid_ele; 
    logic [TileIdBits-1:0]        heap_pre_pid;
    logic [TileIdBits-1:0]        ele_mask, pid_mask;

    logic [TileIdBits-1:0]                    heap_tile_id;

    assign ele_mask = (shift_index == 0) ? 7'b0 : (7'b1111111 >> (TileIdBits-shift_index));
    assign pid_mask = ~ele_mask;

    assign heap_tile_id = address_i[(TileIdBits+ConstantBitsLSB-1):ConstantBitsLSB];

    always_comb begin
      // Default: Unscrambled
      address_o[ConstantBitsLSB-1:0] = address_i[ConstantBitsLSB-1:0];
      address_o[SeqTotalBits-1:ConstantBitsLSB] = {tile_id, scramble};
      address_o[AddrWidth-1:SeqTotalBits] = address_i[AddrWidth-1:SeqTotalBits];
      // If not in bypass mode and address is in sequential region and more than one tile
      if (address_i < (NumTiles * SeqMemSizePerTile)) begin
        address_o[SeqTotalBits-1:ConstantBitsLSB] = {scramble, tile_id};
      end else if ( (address_i >= (TCDMSize - NumTiles * HeapSeqMemSizePerTile)) && (address_i < TCDMSize) ) begin // only in L1 heap region
        // Sequential Heap Logic 
        // 1. `heap_scramble` generation
        heap_scramble  = 'b0;
        heap_pre_pid   = 'b0;
        heap_pid_ele   = 'b0;

        heap_scramble |= address_i >> (ConstantBitsLSB + shift_index);
        heap_pre_pid  |= address_i >> (ConstantBitsLSB + HeapScrambleBits);

        heap_pid_ele  |= (heap_tile_id & ele_mask);
        heap_pid_ele  |= (heap_pre_pid & pid_mask);

        address_o[HeapSeqTotalBits-1:ConstantBitsLSB] = {heap_scramble, heap_pid_ele};

      end
    end

  end

  // Check for unsupported configurations
  if (NumBanksPerTile < 2)
    $fatal(1, "NumBanksPerTile must be greater than 2. The special case '1' is currently not supported!");
  if (SeqMemSizePerTile % (2**ByteOffset*NumBanksPerTile) != 0)
    $fatal(1, "SeqMemSizePerTile must be a multiple of BankWidth*NumBanksPerTile!");
endmodule : address_scrambler
