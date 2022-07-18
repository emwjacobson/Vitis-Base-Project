#include "utils.hpp"

#include <tfhe/tfhe.h>
#include <tfhe/tfhe_io.h>

#include <vector>
#include <unistd.h>
#include <iostream>

int main() {
    FILE* secret_key;
    FILE* answer_data;

    if (!(secret_key = fopen("secret.key", "rb")) || !(answer_data = fopen("answer.data", "rb"))) {
      printf("Error: secret.key and/or answer.data does not exist. Run `./host` and `./cloud` to generate.\n");
      return 1;
    }

    TFheGateBootstrappingSecretKeySet* verify_key = new_tfheGateBootstrappingSecretKeySet_fromFile(secret_key);
    fclose(secret_key);

    const TFheGateBootstrappingParameterSet* verify_params = verify_key->params;
    LweSample* answer = new_gate_bootstrapping_ciphertext_array(DATA_LENGTH, verify_params);


    for(int i = 0; i < DATA_LENGTH; i++) {
        import_gate_bootstrapping_ciphertext_fromFile(answer_data, &answer[i], verify_params);
    }
    fclose(answer_data);

    int16_t int_answer = 0;
    for(int i = 0; i < DATA_LENGTH; i++) {
        int ai = bootsSymDecrypt(&answer[i], verify_key);
        int_answer |= (ai << i);
    }

    printf("The answer is: %X\n", int_answer);

    delete_gate_bootstrapping_ciphertext_array(DATA_LENGTH, answer);
    delete_gate_bootstrapping_secret_keyset(verify_key);
}