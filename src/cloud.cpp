#include "utils.hpp"
#include "cloudcompute.hpp"

#include <tfhe/tfhe.h>
#include <tfhe/tfhe_io.h>

#include <unistd.h>
#include <iostream>
#include <CL/cl2.hpp>

int main() {
  FILE* cloud_key;
  FILE* cloud_data;
  if (!(cloud_key = fopen("cloud.key", "rb")) || !(cloud_data = fopen("cloud.data", "rb"))) {
      printf("Error: cloud.key and/or cloud.data does not exist. Run `./host` to generate.\n");
      return 1;
    return 1;
  }

  // Load the cloud key
  TFheGateBootstrappingCloudKeySet* bk = new_tfheGateBootstrappingCloudKeySet_fromFile(cloud_key);
  fclose(cloud_key);
  const TFheGateBootstrappingParameterSet* cloud_params = bk->params;

  LweSample* a_cipher = new_gate_bootstrapping_ciphertext_array(DATA_LENGTH, cloud_params);
  LweSample* b_cipher = new_gate_bootstrapping_ciphertext_array(DATA_LENGTH, cloud_params);

  // Load data from file into ciphertext arrays
  for(int i=0; i<DATA_LENGTH;i++)
      import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &a_cipher[i], cloud_params);
  for(int i=0; i<DATA_LENGTH;i++)
      import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &b_cipher[i], cloud_params);
  fclose(cloud_data);

  // Allocate space for the result
  LweSample* result = new_gate_bootstrapping_ciphertext_array(DATA_LENGTH, cloud_params);

  // Cloud computation
  CloudCompute cloud(KERNEL_NAME);
  cloud.playground(result, a_cipher, b_cipher, DATA_LENGTH, bk);

  FILE* answer_data = fopen("answer.data", "wb");
  for(int i=0; i<DATA_LENGTH; i++) {
      export_gate_bootstrapping_ciphertext_toFile(answer_data, &result[i], cloud_params);
  }
  fclose(answer_data);

  delete_gate_bootstrapping_ciphertext_array(DATA_LENGTH, result);
  delete_gate_bootstrapping_ciphertext_array(DATA_LENGTH, b_cipher);
  delete_gate_bootstrapping_ciphertext_array(DATA_LENGTH, a_cipher);
  delete_gate_bootstrapping_cloud_keyset(bk);
}