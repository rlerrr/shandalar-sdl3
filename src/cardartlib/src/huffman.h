#ifndef HUFFMAN_H
#define HUFFMAN_H

int Huffman13_Init(uint *bitstream_start, uint *symbol_table, int node_index_base);
int Huffman13_DecodeDwordsWithZeroRuns(uint *out_dwords, uint *bitstream, int bitstream_end);

#endif
