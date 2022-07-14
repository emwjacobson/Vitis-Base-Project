/*
 * Copyright 2020 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#define DATA_SIZE 4096

#include <vector>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <CL/cl2.hpp>
#include <chrono>

#include <tfhe/tfhe.h>
#include <tfhe/tfhe_io.h>

#ifndef KERNEL_NAME
    #error KERNEL_NAME is undefined
    #define KERNEL_NAME ""
#endif

#define MAX_NUMBER 3939
#define MIN_NUMBER 111

// Forward declaration of utility functions included at the end of this file
std::vector<cl::Device> get_xilinx_devices();
char* read_binary_file(const std::string &xclbin_file_name, unsigned &nb);
void gen_keys();
void encrypt_data();

int main(int argc, char** argv) {
// ------------------------------------------------------------------------------------
// Step 1: Initialize the OpenCL environment
// ------------------------------------------------------------------------------------
    cl_int err;
    std::string binaryFile = (argc != 2) ? KERNEL_NAME ".xclbin" : argv[1];
    unsigned fileBufSize;
    std::vector<cl::Device> devices = get_xilinx_devices();
    devices.resize(1);
    cl::Device device = devices[0];
    cl::Context context(device, NULL, NULL, NULL, &err);
    char* fileBuf = read_binary_file(binaryFile, fileBufSize);
    cl::Program::Binaries bins{{fileBuf, fileBufSize}};
    cl::Program program(context, devices, bins, NULL, &err);
    // cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    cl::Kernel krnl(program, KERNEL_NAME, &err);

// ------------------------------------------------------------------------------------
// Step 2: Generate Encryption Keys
// ------------------------------------------------------------------------------------
    printf("Generating encryption keys... ");
    auto start = std::chrono::high_resolution_clock::now();

    gen_keys();

    auto end = std::chrono::high_resolution_clock::now();
    printf("DONE %lims\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());


// ------------------------------------------------------------------------------------
// Step 3: Encrypt Data
// ------------------------------------------------------------------------------------
    printf("Encrypting data... ");
    start = std::chrono::high_resolution_clock::now();

    encrypt_data();

    end = std::chrono::high_resolution_clock::now();
    printf("DONE %lims\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());


// ------------------------------------------------------------------------------------
// Step 4.1: Setup Buffers
// ------------------------------------------------------------------------------------
    printf("Setting up buffers... ");
    start = std::chrono::high_resolution_clock::now();

    FILE* cloud_key = fopen("cloud.key", "rb");
    TFheGateBootstrappingCloudKeySet* bk = new_tfheGateBootstrappingCloudKeySet_fromFile(cloud_key);
    fclose(cloud_key);
    const TFheGateBootstrappingParameterSet* cloud_params = bk->params;
    LweSample* a_cipher = new_gate_bootstrapping_ciphertext_array(16, cloud_params);
    LweSample* b_cipher = new_gate_bootstrapping_ciphertext_array(16, cloud_params);

    FILE* cloud_data = fopen("cloud.data", "rb");
    for(int i = 0; i < 16; i++) {
        import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &a_cipher[i], cloud_params);
    }
    for(int i = 0; i < 16; i++) {
        import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &b_cipher[i], cloud_params);
    }
    fclose(cloud_data);

    LweSample* result = new_gate_bootstrapping_ciphertext_array(16, cloud_params);


    // Create the buffers
    cl::Buffer result_buf(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof(LweSample));
    cl::Buffer a_buf(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, sizeof(LweSample));
    cl::Buffer b_buf(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, sizeof(LweSample));
    cl::Buffer bk_buf(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, sizeof(TFheGateBootstrappingCloudKeySet));

    // Map to host memory
    LweSample* _r = (LweSample*)q.enqueueMapBuffer(result_buf, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, sizeof(LweSample), NULL);
    LweSample* _a = (LweSample*)q.enqueueMapBuffer(a_buf, CL_TRUE, CL_MAP_WRITE, 0, sizeof(LweSample), NULL);
    LweSample* _b = (LweSample*)q.enqueueMapBuffer(b_buf, CL_TRUE, CL_MAP_WRITE, 0, sizeof(LweSample), NULL);
    TFheGateBootstrappingCloudKeySet* _bk = (TFheGateBootstrappingCloudKeySet*)q.enqueueMapBuffer(bk_buf, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, 0, sizeof(TFheGateBootstrappingCloudKeySet), NULL);

    // Copy values from variables to buffer location
    // TODO: Might be able to initialize buffers with host pointer that points to the existing data,
    //       can maybe avoid mapping buffers (except output?) and manually copying data
    memset(_r, 0, sizeof(LweSample));
    memcpy(_a, a_cipher, sizeof(LweSample));
    memcpy(_b, b_cipher, sizeof(LweSample));
    memcpy(_bk, bk, sizeof(TFheGateBootstrappingCloudKeySet));

    end = std::chrono::high_resolution_clock::now();
    printf("DONE %lims\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());


// ------------------------------------------------------------------------------------
// Step 4.2: Execute Kernel
// ------------------------------------------------------------------------------------
    printf("Executing kernel... ");
    start = std::chrono::high_resolution_clock::now();

    krnl.setArg(0, result_buf);
    krnl.setArg(1, a_buf);
    krnl.setArg(2, b_buf);
    krnl.setArg(3, bk_buf);

    q.enqueueMigrateMemObjects({ a_buf, b_buf, bk_buf }, 0);

    q.enqueueTask(krnl);

    q.enqueueMigrateMemObjects({ result_buf }, CL_MIGRATE_MEM_OBJECT_HOST);

    q.finish();

    memcpy(result, _r, sizeof(LweSample));

    FILE* answer_data = fopen("answer.data", "wb");
    for(int i = 0; i < 16; i++) {
        export_gate_bootstrapping_ciphertext_toFile(answer_data, &result[i], cloud_params);
    }
    fclose(answer_data);

    delete_gate_bootstrapping_ciphertext_array(16, result);
    delete_gate_bootstrapping_ciphertext_array(16, b_cipher);
    delete_gate_bootstrapping_ciphertext_array(16, a_cipher);
    delete_gate_bootstrapping_cloud_keyset(bk);

    end = std::chrono::high_resolution_clock::now();
    printf("DONE %lims\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());


// ------------------------------------------------------------------------------------
// Step 5: Verify
// ------------------------------------------------------------------------------------
    printf("Verifying data... ");
    start = std::chrono::high_resolution_clock::now();

    FILE* secret_key = fopen("secret.key", "rb");
    TFheGateBootstrappingSecretKeySet* verify_key = new_tfheGateBootstrappingSecretKeySet_fromFile(secret_key);
    fclose(secret_key);

    const TFheGateBootstrappingParameterSet* verify_params = verify_key->params;
    LweSample* answer = new_gate_bootstrapping_ciphertext_array(16, verify_params);

    answer_data = fopen("answer.data", "rb");
    for(int i = 0; i < 16; i++) {
        import_gate_bootstrapping_ciphertext_fromFile(answer_data, &answer[i], verify_params);
    }
    fclose(answer_data);

    int16_t int_answer = 0;
    for(int i = 0; i < 16; i++) {
        int ai = bootsSymDecrypt(&answer[i], verify_key);
        int_answer |= (ai << i);
    }

    printf("The answer is: %d\n", int_answer);

    delete_gate_bootstrapping_ciphertext_array(16, answer);
    delete_gate_bootstrapping_secret_keyset(verify_key);

    end = std::chrono::high_resolution_clock::now();
    printf("DONE %lims\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());


    delete[] fileBuf;
    return 0;
}



// ------------------------------------------------------------------------------------
// Utility functions
// ------------------------------------------------------------------------------------
std::vector<cl::Device> get_xilinx_devices()
{
    size_t i;
    cl_int err;
    std::vector<cl::Platform> platforms;
    err = cl::Platform::get(&platforms);
    cl::Platform platform;
    for (i  = 0 ; i < platforms.size(); i++){
        platform = platforms[i];
        std::string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err);
        if (platformName == "Xilinx"){
            std::cout << "INFO: Found Xilinx Platform" << std::endl;
            break;
        }
    }
    if (i == platforms.size()) {
        std::cout << "ERROR: Failed to find Xilinx platform" << std::endl;
        exit(EXIT_FAILURE);
    }

    //Getting ACCELERATOR Devices and selecting 1st such device
    std::vector<cl::Device> devices;
    err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
    return devices;
}

char* read_binary_file(const std::string &xclbin_file_name, unsigned &nb)
{
    if(access(xclbin_file_name.c_str(), R_OK) != 0) {
        printf("ERROR: %s xclbin not available please build\n", xclbin_file_name.c_str());
        exit(EXIT_FAILURE);
    }
    //Loading XCL Bin into char buffer
    std::cout << "INFO: Loading '" << xclbin_file_name << "'\n";
    std::ifstream bin_file(xclbin_file_name.c_str(), std::ifstream::binary);
    bin_file.seekg (0, bin_file.end);
    nb = bin_file.tellg();
    bin_file.seekg (0, bin_file.beg);
    char *buf = new char [nb];
    bin_file.read(buf, nb);
    return buf;
}

void gen_keys() {
    const int minimum_lambda = 110;
    TFheGateBootstrappingParameterSet* params = new_default_gate_bootstrapping_parameters(minimum_lambda);

    uint32_t seed[] = { 314, 1337, 1907 };
    tfhe_random_generator_setSeed(seed, 3);
    TFheGateBootstrappingSecretKeySet* key = new_random_gate_bootstrapping_secret_keyset(params);

    FILE* f_secret_key = fopen("secret.key", "wb");
    export_tfheGateBootstrappingSecretKeySet_toFile(f_secret_key, key);
    fclose(f_secret_key);

    FILE* f_cloud_key = fopen("cloud.key", "wb");
    export_tfheGateBootstrappingCloudKeySet_toFile(f_cloud_key, &key->cloud);
    fclose(f_cloud_key);

    delete_gate_bootstrapping_secret_keyset(key);
    delete_gate_bootstrapping_parameters(params);
}

void encrypt_data() {
    FILE* f_secret_key = fopen("secret.key", "rb");
    TFheGateBootstrappingSecretKeySet* secret_key = new_tfheGateBootstrappingSecretKeySet_fromFile(f_secret_key);
    fclose(f_secret_key);

    const TFheGateBootstrappingParameterSet* secret_params = secret_key->params;

    int16_t data1 = MAX_NUMBER;
    int16_t data2 = MIN_NUMBER;

    LweSample* cyphertext1 = new_gate_bootstrapping_ciphertext_array(16, secret_params);
    LweSample* cyphertext2 = new_gate_bootstrapping_ciphertext_array(16, secret_params);

    for(int i = 0; i< 16; i++) {
        bootsSymEncrypt(&cyphertext1[i], (data1 >> i) & 1, secret_key);
        bootsSymEncrypt(&cyphertext2[i], (data2 >> i) & 1, secret_key);
    }

    FILE* cloud_data = fopen("cloud.data", "wb");
    for(int i = 0; i<16; i++) {
        export_gate_bootstrapping_ciphertext_toFile(cloud_data, &cyphertext1[i], secret_params);
    }
    for(int i = 0; i<16; i++) {
        export_gate_bootstrapping_ciphertext_toFile(cloud_data, &cyphertext1[2], secret_params);
    }
    fclose(cloud_data);

    delete_gate_bootstrapping_ciphertext_array(16, cyphertext1);
    delete_gate_bootstrapping_ciphertext_array(16, cyphertext2);

    delete_gate_bootstrapping_secret_keyset(secret_key);
}
