#include <hls_stream.h>
#include "myproject.h"

extern "C" {
void mm2s(
    ap_uint<16>* in,      // Raw 16-bit data from host
    hls::stream<input_t>& out,  // Stream to CNN
    int size
) {
    #pragma HLS INTERFACE m_axi port=in offset=slave bundle=gmem0
    #pragma HLS INTERFACE axis port=out
    #pragma HLS INTERFACE s_axilite port=size
    #pragma HLS INTERFACE s_axilite port=in
    #pragma HLS INTERFACE s_axilite port=return

mm2s_loop:
    for(int i = 0; i < size; i++) {
        #pragma HLS PIPELINE II=1
        input_t tmp;
        // Reinterpret raw bits as ap_fixed<16,6>
        ap_fixed<16,6> fixed_val;
        fixed_val.range() = in[i];
        tmp[0] = fixed_val;
        out.write(tmp);
    }
}
}
