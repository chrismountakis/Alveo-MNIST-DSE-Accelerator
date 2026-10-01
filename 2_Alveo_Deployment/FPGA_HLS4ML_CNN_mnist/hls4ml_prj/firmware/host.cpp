#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <cstdint>

// XRT C++ headers
#include <xrt/xrt_device.h>
#include <xrt/xrt_kernel.h>
#include <xrt/xrt_bo.h>
#include <xrt/xrt_uuid.h>

#define IMAGE_WIDTH    28
#define IMAGE_HEIGHT   28
#define IMAGE_CHANNELS 1
#define INPUT_SIZE     (IMAGE_WIDTH * IMAGE_HEIGHT * IMAGE_CHANNELS)
#define OUTPUT_SIZE    10
#define NUM_IMAGES     20

// ----------------------
// Conversion functions
// ----------------------
uint16_t float_to_apfixed16_6(float val) {
    int16_t fixed = static_cast<int16_t>(val * 1024.0f); // 2^10 scale factor
    return static_cast<uint16_t>(fixed);
}

float apfixed29_13_to_float(uint32_t raw) {
    int32_t val = static_cast<int32_t>(raw);
    if (val & 0x10000000) val |= 0xE0000000; // sign-extend from 29 bits
    return static_cast<float>(val) / 65536.0f; // 2^16 scale factor
}

// ----------------------
// Helper functions
// ----------------------
std::vector<float> load_mnist_image(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    std::vector<float> image(INPUT_SIZE);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return image;
    }
    file.read(reinterpret_cast<char*>(image.data()), INPUT_SIZE * sizeof(float));
    return image;
}

std::vector<int> load_labels(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<int> labels;
    int label;
    while (file >> label) labels.push_back(label);
    return labels;
}

// ----------------------
// Main
// ----------------------
int main(int argc, char** argv) {
    bool verbose = false;
    std::string binaryFile;

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File> [--verbose | -v]" << std::endl;
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--verbose") == 0 || std::strcmp(argv[i], "-v") == 0)
            verbose = true;
        else
            binaryFile = argv[i];
    }

    if (binaryFile.empty()) {
        std::cerr << "Error: Missing XCLBIN file argument." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Loading ground truth labels..." << std::endl;
    std::vector<int> true_labels = load_labels("test_images/labels.txt");

    if (true_labels.size() != NUM_IMAGES) {
        std::cerr << "Error: Expected " << NUM_IMAGES << " labels, got " << true_labels.size() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "✓ Loaded " << true_labels.size() << " labels" << std::endl;

    std::cout << "Initializing XRT..." << std::endl;
    xrt::device device(0);
    xrt::uuid uuid = device.load_xclbin(binaryFile);
    std::cout << "✓ XCLBIN loaded" << std::endl;

    std::cout << "Loading kernels..." << std::endl;
    xrt::kernel krnl_mm2s(device, uuid, "mm2s");
    xrt::kernel krnl_myproject(device, uuid, "myproject");
    xrt::kernel krnl_s2mm(device, uuid, "s2mm");
    std::cout << "✓ Kernels loaded" << std::endl;

    size_t input_size_bytes = INPUT_SIZE * sizeof(uint16_t);
    size_t output_size_bytes = OUTPUT_SIZE * sizeof(uint32_t);

    xrt::bo bo_input(device, input_size_bytes, krnl_mm2s.group_id(0));
    xrt::bo bo_output(device, output_size_bytes, krnl_s2mm.group_id(1));
    auto bo_input_map = bo_input.map<uint16_t*>();
    auto bo_output_map = bo_output.map<uint32_t*>();
    std::cout << "✓ Buffers allocated and mapped" << std::endl;

    int correct_predictions = 0;
    double total_inference_time = 0.0;

    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "Running inference on " << NUM_IMAGES << " MNIST test images..." << std::endl;
    std::cout << std::string(70, '=') << "\n" << std::endl;

    for (int img_idx = 0; img_idx < NUM_IMAGES; ++img_idx) {
        std::cout << "=== Image " << img_idx << " === " << std::flush;
        std::string image_file = "test_images/image_" + std::to_string(img_idx) + ".bin";

        std::vector<float> input_image = load_mnist_image(image_file);
        if (verbose) std::cout << "\n[1] Image loaded (" << input_image.size() << " floats)" << std::endl;

        for (int i = 0; i < INPUT_SIZE; ++i)
            bo_input_map[i] = float_to_apfixed16_6(input_image[i]);
        if (verbose) std::cout << "[2] Converted float → fixed" << std::endl;

        bo_input.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        if (verbose) std::cout << "[3] Input synced" << std::endl;

        auto start = std::chrono::high_resolution_clock::now();

        if (verbose) std::cout << "[4] Starting kernels..." << std::endl;
        xrt::run run_mm2s(krnl_mm2s);
        xrt::run run_myproject(krnl_myproject);
        xrt::run run_s2mm(krnl_s2mm);

        run_mm2s.set_arg(0, bo_input);
        run_mm2s.set_arg(2, INPUT_SIZE);
        run_s2mm.set_arg(1, bo_output);
        run_s2mm.set_arg(2, 1);

        run_mm2s.start();
        run_myproject.start();
        run_s2mm.start();

        run_mm2s.wait();
        run_myproject.wait();
        run_s2mm.wait();

        auto end = std::chrono::high_resolution_clock::now();
        total_inference_time += std::chrono::duration<double>(end - start).count();

        bo_output.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        if (verbose) std::cout << "[5] Output synced" << std::endl;

        float max_score = -1e9f;
        int predicted_class = -1;
        for (int i = 0; i < OUTPUT_SIZE; ++i) {
            float score = apfixed29_13_to_float(bo_output_map[i]);
            if (score > max_score) {
                max_score = score;
                predicted_class = i;
            }
        }

        bool is_correct = (predicted_class == true_labels[img_idx]);
        if (is_correct) ++correct_predictions;

        std::cout << "True: " << true_labels[img_idx]
                  << " | Predicted: " << predicted_class
                  << " | " << (is_correct ? "✓" : "✗")
                  << " | " << std::fixed << std::setprecision(3)
                  << std::chrono::duration<double>(end - start).count() * 1000 << " ms" << std::endl;

        if (!is_correct) {
            std::vector<std::pair<float, int>> scores;
            for (int i = 0; i < OUTPUT_SIZE; ++i)
                scores.emplace_back(apfixed29_13_to_float(bo_output_map[i]), i);
            std::sort(scores.rbegin(), scores.rend());

            std::cout << "         Top-3: ";
            for (int i = 0; i < 3; ++i)
                std::cout << scores[i].second << "(" << std::fixed << std::setprecision(2)
                          << scores[i].first << ") ";
            std::cout << std::endl;
        }
    }

    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "RESULTS SUMMARY" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "Total images:        " << NUM_IMAGES << std::endl;
    std::cout << "Correct predictions: " << correct_predictions << std::endl;
    std::cout << "Wrong predictions:   " << (NUM_IMAGES - correct_predictions) << std::endl;
    std::cout << "Accuracy:            " << std::fixed << std::setprecision(2)
              << (100.0 * correct_predictions / NUM_IMAGES) << "%" << std::endl;
    std::cout << "Average time/image:  " << std::fixed << std::setprecision(3)
              << (total_inference_time * 1000 / NUM_IMAGES) << " ms" << std::endl;
    std::cout << "Total time:          " << std::fixed << std::setprecision(3)
              << (total_inference_time * 1000) << " ms" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    if (correct_predictions == NUM_IMAGES)
        std::cout << "\n✓ ALL TESTS PASSED!" << std::endl;
    else
        std::cout << "\n✗ Some predictions were incorrect." << std::endl;

    return EXIT_SUCCESS;
}
