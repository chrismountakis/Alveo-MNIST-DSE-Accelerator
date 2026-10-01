# Alveo U250 Hardware-Accelerated CNN (MNIST)

This repository is an end-to-end hardware acceleration pipeline for a CNN on MNIST. It spans Design Space Exploration (DSE) to physical FPGA deployment on a Xilinx Alveo U250 Data Center Accelerator Card using `hls4ml` and Vitis HLS.

## Project Structure

The project is divided into two phases:

### 1. Design Space Exploration (`1_Design_Space_Exploration/`)
Before deploying to hardware, we need the optimal neural network architecture. This folder contains the research phase:
- We swept across multiple precision bit-widths (from a 16-bit baseline down to 2-bit using **QKeras**).
- We experimented with unstructured sparsity levels (0%, 50%, 70%, 90%) using TensorFlow Model Optimization (`tfmot`).
- We varied the hardware Reuse Factor (RF) from 1 to 2048 to analyze the latency vs. DSP/LUT tradeoffs.
- **Result:** We found that a **4-bit, 70% sparse** model with `RF=1` is the Pareto-optimal "Golden Model" for our target latency and resource budgets. 
- *Detailed analysis can be found in the IEEE double-column report located in the root of the repository.*

### 2. Physical Alveo U250 Deployment (`2_Alveo_Deployment/`)
To deploy one of these quantized networks, we took a baseline 8-bit model and compiled it through the full Xilinx Vitis toolchain:
- Used **hls4ml** to bridge the gap between Keras and C++ HLS.
- Compiled the OpenCL host application (`myproject_host/`) to stream images from the CPU to the FPGA over PCI-e.
- Routed the design for the **xcu250-figd2104-2L-e** board (Xilinx Alveo U250).
- *Compilation logs and synthesis reports (`.log` and `.rpt`) from Place & Route are included in the repository. (The 100MB+ `.xclbin` bitstream binaries are excluded via `.gitignore`.)*

## Key Technologies Used
* **TensorFlow & Keras:** Base model training and pruning (`tfmot`).
* **QKeras:** Quantization-Aware Training (QAT).
* **hls4ml:** Automated conversion from Keras models to C++ High-Level Synthesis code.
* **Xilinx Vitis & Vivado HLS:** Hardware synthesis and Place & Route.
* **OpenCL & C++:** Host-side driver application development.
