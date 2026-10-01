# Design Files - DSE CNN MNIST

This folder contains the core design and automation files for our Design Space Exploration (DSE) project using hls4ml.

## Files Included:
- **DSE_CNN_mnist.ipynb**: The main Jupyter Notebook containing the end-to-end pipeline. It includes:
  - Data loading and preprocessing.
  - Floating-point baseline CNN training.
  - Magnitude-based unstructured pruning sweeps (0%, 50%, 70%, 90%).
  - Post-Training Quantization (PTQ) sweeps using QKeras (2-bit to 8-bit).
  - Automated hls4ml C++ hardware generation and Vivado HLS synthesis.
  - Python-based Pareto curve plotting and report generation.

## Dependencies:
- TensorFlow / Keras
- TensorFlow Model Optimization (tfmot)
- QKeras
- hls4ml
- Xilinx Vivado HLS (for synthesis)

## How to Run:
The notebook is structured sequentially. If `TRAIN = True`, the notebook will actively train the models and synthesize the hardware via hls4ml. If `TRAIN = False`, it will load pre-computed accuracy scores and hardware metrics from disk to immediately generate the Pareto curves and tables.
