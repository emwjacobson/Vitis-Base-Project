#ifndef __CLOUD_HPP__
#define __CLOUD_HPP__

#include <tfhe/tfhe.h>
#include <tfhe/tfhe_io.h>

class CloudCompute {
public:
  CloudCompute(std::string binaryFile);
  void playground(LweSample* result, const LweSample* a, const LweSample* b, const int nb_bits, const TFheGateBootstrappingCloudKeySet* bk);
private:
  void __playground_cpu(LweSample* result, const LweSample* a, const LweSample* b, const int nb_bits, const TFheGateBootstrappingCloudKeySet* bk);
  void __playground_fpga(LweSample* result, const LweSample* a, const LweSample* b, const int nb_bits, const TFheGateBootstrappingCloudKeySet* bk);

  // Gates
  void __bootsCOPY(LweSample *result, const LweSample *ca, const TFheGateBootstrappingCloudKeySet *bk);
  void __bootsAND(LweSample *result, const LweSample *ca, const LweSample *cb, const TFheGateBootstrappingCloudKeySet *bk);

  // LWE Functions
  void __lweCopy(LweSample* result, const LweSample* sample, const LweParams* params);
  void __lweAddTo(LweSample* result, const LweSample* sample, const LweParams* params);
  void __lweNoiselessTrivial(LweSample* result, Torus32 mu, const LweParams* params);

  // FFT Functions
  void __tfhe_bootstrap_FFT(LweSample *result, const LweBootstrappingKeyFFT *bk, Torus32 mu, const LweSample *x);
  void __tfhe_bootstrap_woKS_FFT(LweSample *result, const LweBootstrappingKeyFFT *bk, Torus32 mu, const LweSample *x);
  void __tfhe_blindRotateAndExtract_FFT(LweSample *result, const TorusPolynomial *v, const TGswSampleFFT *bk, const int32_t barb,
                                        const int32_t *bara, const int32_t n, const TGswParams *bk_params);
  void __tfhe_blindRotate_FFT(TLweSample *accum, const TGswSampleFFT *bkFFT, const int32_t *bara,
                              const int32_t n, const TGswParams *bk_params);
};

#endif