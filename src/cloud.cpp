#include "cloud.hpp"
#include "utils.hpp"
#include <chrono>

CloudCompute::CloudCompute() {
  // TODO: Setup OpenCL stuff here
}

void CloudCompute::minimum(LweSample* result, const LweSample* a, const LweSample* b, const int nb_bits, const TFheGateBootstrappingCloudKeySet* bk) {
  int64_t time;

  time = time_function<std::chrono::microseconds>([this, result, a, b, nb_bits, bk]() -> void {
          __minimum_fpga(result, a, b, nb_bits, bk);
        });
  printf("FPGA Kernel Took %lius\n", time);

  time = time_function<std::chrono::microseconds>([this, result, a, b, nb_bits, bk]() -> void {
          __minimum_cpu(result, a, b, nb_bits, bk);
        });
  printf("CPU Kernel Took %lius\n", time);

}

void CloudCompute::__minimum_cpu(LweSample* result, const LweSample* a, const LweSample* b, const int nb_bits, const TFheGateBootstrappingCloudKeySet* bk) {
  for(int i=0; i<nb_bits; i++) {
    bootsCOPY(&result[i], &a[i], bk);
  }
}

void CloudCompute::__minimum_fpga(LweSample* result, const LweSample* a, const LweSample* b, const int nb_bits, const TFheGateBootstrappingCloudKeySet* bk) {
  for(int i=0; i<nb_bits; i++) {
    __bootsCOPY(&result[i], &a[i], bk);
  }
}

// ------------------------------------------------------------------------------------
// Gates
// ------------------------------------------------------------------------------------
void CloudCompute::__bootsCOPY(LweSample *result, const LweSample *ca, const TFheGateBootstrappingCloudKeySet *bk) {
  const LweParams *in_out_params = bk->params->in_out_params;
  __lweCopy(result, ca, in_out_params);
}

// ------------------------------------------------------------------------------------
// LWE Functions
// ------------------------------------------------------------------------------------
void CloudCompute::__lweCopy(LweSample* result, const LweSample* sample, const LweParams* params) {
  const int32_t n = params->n;

  for (int32_t i = 0; i < n; ++i) result->a[i] = sample->a[i];
  result->b = sample->b;
  result->current_variance = sample->current_variance;
}









// // ------------------------------------------------------------------------------------
// // Main program
// // ------------------------------------------------------------------------------------
// int main_old(int argc, char** argv)
// {
// // ------------------------------------------------------------------------------------
// // Step 1: Initialize the OpenCL environment
// // ------------------------------------------------------------------------------------
//     cl_int err;
//     std::string binaryFile = (argc != 2) ? "maths.xclbin" : argv[1];
//     unsigned fileBufSize;
//     std::vector<cl::Device> devices = get_xilinx_devices();
//     devices.resize(1);
//     cl::Device device = devices[0];
//     cl::Context context(device, NULL, NULL, NULL, &err);
//     char* fileBuf = read_binary_file(binaryFile, fileBufSize);
//     cl::Program::Binaries bins{{fileBuf, fileBufSize}};
//     cl::Program program(context, devices, bins, NULL, &err);
//     cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
//     cl::Kernel krnl_vector_add(program, "vadd", &err);
//     cl::Kernel krnl_vector_sub(program, "vsub", &err);

// // ------------------------------------------------------------------------------------
// // Step 2: Create buffers and initialize test values
// // ------------------------------------------------------------------------------------
//     // Create the buffers and allocate memory
//     cl::Buffer in1_buf(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY,  sizeof(int) * DATA_SIZE, NULL, &err);
//     cl::Buffer in2_buf(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY,  sizeof(int) * DATA_SIZE, NULL, &err);
//     cl::Buffer add_buf(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof(int) * DATA_SIZE, NULL, &err);
//     cl::Buffer sub_buf(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof(int) * DATA_SIZE, NULL, &err);

//     // Map host-side buffer memory to user-space pointers
//     int *in1 = (int *)q.enqueueMapBuffer(in1_buf, CL_TRUE, CL_MAP_WRITE, 0, sizeof(int) * DATA_SIZE, NULL);
//     int *in2 = (int *)q.enqueueMapBuffer(in2_buf, CL_TRUE, CL_MAP_WRITE, 0, sizeof(int) * DATA_SIZE, NULL);
//     int *out_add = (int *)q.enqueueMapBuffer(add_buf, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, 0, sizeof(int) * DATA_SIZE, NULL);
//     int *out_sub = (int *)q.enqueueMapBuffer(sub_buf, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, 0, sizeof(int) * DATA_SIZE, NULL);

//     // Initialize the vectors used in the test
//     for(int i = 0 ; i < DATA_SIZE ; i++){
//         in1[i] = rand() % DATA_SIZE;
//         in2[i] = rand() % DATA_SIZE;
//         out_add[i] = 0;
//         out_sub[i] = 0;
//     }

// // ------------------------------------------------------------------------------------
// // Step 3: Run the kernel
// // ------------------------------------------------------------------------------------
//     // Set kernel arguments
//     krnl_vector_add.setArg(0, in1_buf);
//     krnl_vector_add.setArg(1, in2_buf);
//     krnl_vector_add.setArg(2, add_buf);
//     krnl_vector_add.setArg(3, DATA_SIZE);

//     // Schedule transfer of inputs to device memory, execution of kernel, and transfer of outputs back to host memory
//     q.enqueueMigrateMemObjects({in1_buf, in2_buf}, 0 /* 0 means from host*/);
//     q.enqueueTask(krnl_vector_add);
//     q.enqueueMigrateMemObjects({add_buf}, CL_MIGRATE_MEM_OBJECT_HOST);

//     // Set kernel arguments
//     krnl_vector_sub.setArg(0, in1_buf);
//     krnl_vector_sub.setArg(1, in2_buf);
//     krnl_vector_sub.setArg(2, sub_buf);
//     krnl_vector_sub.setArg(3, DATA_SIZE);

//     // Schedule transfer of inputs to device memory, execution of kernel, and transfer of outputs back to host memory
//     q.enqueueMigrateMemObjects({in1_buf, in2_buf}, 0 /* 0 means from host*/);
//     q.enqueueTask(krnl_vector_sub);
//     q.enqueueMigrateMemObjects({sub_buf}, CL_MIGRATE_MEM_OBJECT_HOST);

//     // Wait for all scheduled operations to finish
//     q.finish();

// // ------------------------------------------------------------------------------------
// // Step 4: Check Results and Release Allocated Resources
// // ------------------------------------------------------------------------------------
//     bool match = true;
//     for (int i = 0 ; i < DATA_SIZE ; i++){
//         int expected_add = in1[i]+in2[i];
//         int expected_sub = in1[i]-in2[i];
//         if (out_add[i] != expected_add || out_sub[i] != expected_sub){
//             std::cout << "Error: Result mismatch" << std::endl;
//             std::cout << "i = " << i << " CPU result = " << expected_add << " Device result = " << out_add[i] << std::endl;
//             std::cout << "i = " << i << " CPU result = " << expected_sub << " Device result = " << out_sub[i] << std::endl;
//             match = false;
//             break;
//         }
//     }

//     delete[] fileBuf;

//     std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;
//     return (match ? EXIT_SUCCESS : EXIT_FAILURE);
// }