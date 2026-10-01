#include <hls_stream.h>
#include "myproject.h"

typedef ap_uint<32> data_word;

extern "C" {
void s2mm(
    hls::stream<result_t>& in,
    data_word* out,
    int size  // Number of result_t packets (should be 1)
) {
#pragma HLS INTERFACE axis port=in
#pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem1
#pragma HLS INTERFACE s_axilite port=out bundle=control
#pragma HLS INTERFACE s_axilite port=size bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

s2mm_loop:
    for(int i = 0; i < size; i++) {
#pragma HLS PIPELINE OFF
        result_t tmp = in.read();  // Read 1 packet (10 scores)
        // Extract all 10 scores from the array
        for(int j = 0; j < 10; j++) {
#pragma HLS PIPELINE II=1
            ap_fixed<29,13> score = tmp[j];
            ap_int<29> bits = score.range();  // Get bit representation
            out[i*10 + j] = bits;
        }
    }
}
}
