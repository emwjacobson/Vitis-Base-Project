#include "cloudcompute.hpp"
#include "utils.hpp"
#include <chrono>
#include <string>
#include <CL/cl2.hpp>

CloudCompute::CloudCompute(std::string binaryFile)
{
  // ------------------------------------------------------------------------------------
  // Step 1: Initialize the OpenCL environment
  // ------------------------------------------------------------------------------------
  // cl_int err;
  // unsigned fileBufSize;
  // std::vector<cl::Device> devices = get_xilinx_devices();
  // devices.resize(1);
  // cl::Device device = devices[0];
  // cl::Context context(device, NULL, NULL, NULL, &err);
  // char *fileBuf = read_binary_file(binaryFile + ".xclbin", fileBufSize);
  // cl::Program::Binaries bins{{fileBuf, fileBufSize}};
  // cl::Program program(context, devices, bins, NULL, &err);
  // cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
  // cl::Kernel krnl_vector_add(program, "vadd", &err);
}

void CloudCompute::playground(LweSample *result, const LweSample *a, const LweSample *b, const int nb_bits, const TFheGateBootstrappingCloudKeySet *bk)
{
  // auto time = time_function<std::chrono::milliseconds>([this, result, a, b, nb_bits, bk]() -> void {
  //         __playground_cpu(result, a, b, nb_bits, bk);
  //       });
  // printf("CPU Kernel Took %lims\n", time);

  // time = time_function<std::chrono::milliseconds>([this, result, a, b, nb_bits, bk]() -> void {
  //         __playground_fpga(result, a, b, nb_bits, bk);
  //       });
  // printf("FPGA Kernel Took %lims\n", time);

  __bootsAND(result, a, b, bk);
}

void CloudCompute::__playground_cpu(LweSample *result, const LweSample *a, const LweSample *b, const int nb_bits, const TFheGateBootstrappingCloudKeySet *bk)
{
  for (int i = 0; i < nb_bits; i++)
  {
    bootsAND(&result[i], &a[i], &b[i], bk);
  }
}

void CloudCompute::__playground_fpga(LweSample *result, const LweSample *a, const LweSample *b, const int nb_bits, const TFheGateBootstrappingCloudKeySet *bk)
{
  for (int i = 0; i < nb_bits; i++)
  {
    __bootsAND(&result[i], &a[i], &b[i], bk);
  }
}

// ------------------------------------------------------------------------------------
// Gates : These probably cant be ported to HLS (without major work), as the CloudKeySet is a mess of pointers
// ------------------------------------------------------------------------------------
void CloudCompute::__bootsCOPY(LweSample *result, const LweSample *ca, const TFheGateBootstrappingCloudKeySet *bk)
{
  const LweParams *in_out_params = bk->params->in_out_params;
  __lweCopy(result, ca, in_out_params);
}

void CloudCompute::__bootsAND(LweSample *result, const LweSample *ca, const LweSample *cb, const TFheGateBootstrappingCloudKeySet *bk)
{
  static const Torus32 MU = modSwitchToTorus32(1, 8);
  const LweParams *in_out_params = bk->params->in_out_params;

  LweSample *temp_result = new_LweSample(in_out_params);

  // compute: (0,-1/8) + ca + cb
  static const Torus32 AndConst = modSwitchToTorus32(-1, 8);
  __lweNoiselessTrivial(temp_result, AndConst, in_out_params);
  __lweAddTo(temp_result, ca, in_out_params);
  __lweAddTo(temp_result, cb, in_out_params);

  // if the phase is positive, the result is 1/8
  // if the phase is positive, else the result is -1/8
  __tfhe_bootstrap_FFT(result, bk->bkFFT, MU, temp_result);

  delete_LweSample(temp_result);
}

// ------------------------------------------------------------------------------------
// LWE Functions : These should be easily ported to HLS
// ------------------------------------------------------------------------------------
void CloudCompute::__lweCopy(LweSample *result, const LweSample *sample, const LweParams *params)
{
  const int32_t n = params->n;

  for (int32_t i = 0; i < n; ++i)
    result->a[i] = sample->a[i];
  result->b = sample->b;
  result->current_variance = sample->current_variance;
}

void CloudCompute::__lweAddTo(LweSample *result, const LweSample *sample, const LweParams *params)
{
  const int32_t n = params->n;

  for (int32_t i = 0; i < n; ++i)
    result->a[i] += sample->a[i];
  result->b += sample->b;
  result->current_variance += sample->current_variance;
}

void CloudCompute::__lweNoiselessTrivial(LweSample *result, Torus32 mu, const LweParams *params)
{
  const int32_t n = params->n;

  for (int32_t i = 0; i < n; ++i)
    result->a[i] = 0;
  result->b = mu;
  result->current_variance = 0.;
}

// ------------------------------------------------------------------------------------
// FFT Functions : Can these be ported to HLS?!?
// ------------------------------------------------------------------------------------
void CloudCompute::__tfhe_bootstrap_FFT(LweSample *result, const LweBootstrappingKeyFFT *bk, Torus32 mu, const LweSample *x)
{
  LweSample *u = new_LweSample(&bk->accum_params->extracted_lweparams);

  __tfhe_bootstrap_woKS_FFT(u, bk, mu, x); // BENCHMARK: 48729us (93%)

  // Key switching
  lweKeySwitch(result, bk->ks, u); // BENCHMARK: 3679us (7%)

  delete_LweSample(u);
}

void CloudCompute::__tfhe_bootstrap_woKS_FFT(LweSample *result, const LweBootstrappingKeyFFT *bk, Torus32 mu, const LweSample *x) {
    const TGswParams *bk_params = bk->bk_params;
    const TLweParams *accum_params = bk->accum_params;
    const LweParams *in_params = bk->in_out_params;
    const int32_t N = accum_params->N;
    const int32_t Nx2 = 2 * N;
    const int32_t n = in_params->n;

    TorusPolynomial *testvect = new_TorusPolynomial(N);
    int32_t *bara = new int32_t[N];


    // Modulus switching
    int32_t barb = modSwitchFromTorus32(x->b, Nx2);
    for (int32_t i = 0; i < n; i++) {
        bara[i] = modSwitchFromTorus32(x->a[i], Nx2);
    }

    // the initial testvec = [mu,mu,mu,...,mu]
    for (int32_t i = 0; i < N; i++) testvect->coefsT[i] = mu;

    // Bootstrapping rotation and extraction
    __tfhe_blindRotateAndExtract_FFT(result, testvect, bk->bkFFT, barb, bara, n, bk_params); // BENCHMARK: 48715us


    delete[] bara;
    delete_TorusPolynomial(testvect);
}

void CloudCompute::__tfhe_blindRotateAndExtract_FFT(LweSample *result, const TorusPolynomial *v, const TGswSampleFFT *bk, const int32_t barb,
                                        const int32_t *bara, const int32_t n, const TGswParams *bk_params) {

    const TLweParams *accum_params = bk_params->tlwe_params;
    const LweParams *extract_params = &accum_params->extracted_lweparams;
    const int32_t N = accum_params->N;
    const int32_t _2N = 2 * N;

    // Test polynomial
    TorusPolynomial *testvectbis = new_TorusPolynomial(N);
    // Accumulator
    TLweSample *acc = new_TLweSample(accum_params);

    int64_t time;

    // testvector = X^{2N-barb}*v
    if (barb != 0) {
      torusPolynomialMulByXai(testvectbis, _2N - barb, v); // BENCHMARK: 10us
    } else {
      torusPolynomialCopy(testvectbis, v);
    }

    tLweNoiselessTrivial(acc, testvectbis, accum_params);
    // Blind rotation
    tfhe_blindRotate_FFT(acc, bk, bara, n, bk_params); // BENCHMARK: 52947us
    // Extraction
    tLweExtractLweSample(result, acc, extract_params, accum_params); // BENCHMARK: 7us

    delete_TLweSample(acc);
    delete_TorusPolynomial(testvectbis);
}

// void CloudCompute::__tfhe_blindRotate_FFT(TLweSample *accum,
//                                  const TGswSampleFFT *bkFFT,
//                                  const int32_t *bara,
//                                  const int32_t n,
//                                  const TGswParams *bk_params) {

//     //TGswSampleFFT* temp = new_TGswSampleFFT(bk_params);
//     TLweSample *temp = new_TLweSample(bk_params->tlwe_params);
//     TLweSample *temp2 = temp;
//     TLweSample *temp3 = accum;

//     for (int32_t i = 0; i < n; i++) {
//         const int32_t barai = bara[i];
//         if (barai == 0) continue; //indeed, this is an easy case!

//         tfhe_MuxRotate_FFT(temp2, temp3, bkFFT + i, barai, bk_params);
//         swap(temp2, temp3);
//     }
//     if (temp3 != accum) {
//         tLweCopy(accum, temp3, bk_params->tlwe_params);
//     }

//     delete_TLweSample(temp);
//     //delete_TGswSampleFFT(temp);
// }


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
