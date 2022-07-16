#ifndef __CLOUD_HPP__
#define __CLOUD_HPP__

#include <tfhe/tfhe.h>
#include <tfhe/tfhe_io.h>

class CloudCompute {
public:
  CloudCompute();
  void minimum(LweSample* result, const LweSample* a, const LweSample* b, const int nb_bits, const TFheGateBootstrappingCloudKeySet* bk);
private:
  void __minimum_cpu(LweSample* result, const LweSample* a, const LweSample* b, const int nb_bits, const TFheGateBootstrappingCloudKeySet* bk);
  void __minimum_fpga(LweSample* result, const LweSample* a, const LweSample* b, const int nb_bits, const TFheGateBootstrappingCloudKeySet* bk);

  // Gates
  void __bootsCOPY(LweSample *result, const LweSample *ca, const TFheGateBootstrappingCloudKeySet *bk);

  // LWE Functions
  void __lweCopy(LweSample* result, const LweSample* sample, const LweParams* params);
};

#endif