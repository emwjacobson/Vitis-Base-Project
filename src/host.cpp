#include "utils.hpp"
#include "cloud.hpp"

#include <tfhe/tfhe.h>
#include <tfhe/tfhe_io.h>

#include <vector>
#include <unistd.h>
#include <iostream>
#include <CL/cl2.hpp>


#define MAX_NUMBER 3939
#define MIN_NUMBER 111
#define DATA_LENGTH 16

#define KERNEL_NAME "maths"

// Forward declaration of utility functions included at the end of this file
void gen_keys();
void encrypt_data();
void cloud();
void verify_data();

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0); // Disable stdout buffering

// ------------------------------------------------------------------------------------
// Step 1: Generate Encryption Keys
// ------------------------------------------------------------------------------------
    printf("Generating encryption keys... ");
    int64_t time = time_function(gen_keys);
    printf("DONE %lims\n", time);


// ------------------------------------------------------------------------------------
// Step 2: Encrypt Data
// ------------------------------------------------------------------------------------
    printf("Encrypting data... ");
    time = time_function(encrypt_data);
    printf("DONE %lims\n", time);


// ------------------------------------------------------------------------------------
// Step 3: Perform "Cloud" Function
// ------------------------------------------------------------------------------------
    printf("Performing \"cloud\" operations...\n");
    time = time_function(cloud);
    printf("DONE %lims\n", time);


// ------------------------------------------------------------------------------------
// Step 4: Verify
// ------------------------------------------------------------------------------------
    printf("Verifying data... ");
    time = time_function(verify_data);
    printf("DONE %lims\n", time);
}



// ------------------------------------------------------------------------------------
// Helper Functions
// ------------------------------------------------------------------------------------
void gen_keys() {
    int minimum_lambda = 110;
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

    LweSample* cyphertext1 = new_gate_bootstrapping_ciphertext_array(DATA_LENGTH, secret_params);
    LweSample* cyphertext2 = new_gate_bootstrapping_ciphertext_array(DATA_LENGTH, secret_params);

    for(int i = 0; i< DATA_LENGTH; i++) {
        bootsSymEncrypt(&cyphertext1[i], (data1 >> i) & 1, secret_key);
        bootsSymEncrypt(&cyphertext2[i], (data2 >> i) & 1, secret_key);
    }

    FILE* cloud_data = fopen("cloud.data", "wb");
    for(int i = 0; i<DATA_LENGTH; i++) {
        export_gate_bootstrapping_ciphertext_toFile(cloud_data, &cyphertext1[i], secret_params);
    }
    for(int i = 0; i<DATA_LENGTH; i++) {
        export_gate_bootstrapping_ciphertext_toFile(cloud_data, &cyphertext2[i], secret_params);
    }
    fclose(cloud_data);

    delete_gate_bootstrapping_ciphertext_array(DATA_LENGTH, cyphertext1);
    delete_gate_bootstrapping_ciphertext_array(DATA_LENGTH, cyphertext2);
    delete_gate_bootstrapping_secret_keyset(secret_key);
}

void cloud() {
    // Load the cloud key
    FILE* cloud_key = fopen("cloud.key", "rb");
    TFheGateBootstrappingCloudKeySet* bk = new_tfheGateBootstrappingCloudKeySet_fromFile(cloud_key);
    fclose(cloud_key);
    const TFheGateBootstrappingParameterSet* cloud_params = bk->params;

    LweSample* a_cipher = new_gate_bootstrapping_ciphertext_array(DATA_LENGTH, cloud_params);
    LweSample* b_cipher = new_gate_bootstrapping_ciphertext_array(DATA_LENGTH, cloud_params);

    // Load data from file into ciphertext arrays
    FILE* cloud_data = fopen("cloud.data", "rb");
    for(int i=0; i<DATA_LENGTH;i++)
        import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &a_cipher[i], cloud_params);
    for(int i=0; i<DATA_LENGTH;i++)
        import_gate_bootstrapping_ciphertext_fromFile(cloud_data, &b_cipher[i], cloud_params);
    fclose(cloud_data);

    // Allocate space for the result
    LweSample* result = new_gate_bootstrapping_ciphertext_array(DATA_LENGTH, cloud_params);

    // Cloud computation
    CloudCompute cloud(KERNEL_NAME);
    cloud.minimum(result, a_cipher, b_cipher, DATA_LENGTH, bk);

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

void verify_data() {
    FILE* secret_key = fopen("secret.key", "rb");
    TFheGateBootstrappingSecretKeySet* verify_key = new_tfheGateBootstrappingSecretKeySet_fromFile(secret_key);
    fclose(secret_key);

    const TFheGateBootstrappingParameterSet* verify_params = verify_key->params;
    LweSample* answer = new_gate_bootstrapping_ciphertext_array(DATA_LENGTH, verify_params);

    FILE* answer_data = fopen("answer.data", "rb");
    for(int i = 0; i < DATA_LENGTH; i++) {
        import_gate_bootstrapping_ciphertext_fromFile(answer_data, &answer[i], verify_params);
    }
    fclose(answer_data);

    int16_t int_answer = 0;
    for(int i = 0; i < DATA_LENGTH; i++) {
        int ai = bootsSymDecrypt(&answer[i], verify_key);
        int_answer |= (ai << i);
    }

    printf("The answer is: %d\n", int_answer);

    delete_gate_bootstrapping_ciphertext_array(DATA_LENGTH, answer);
    delete_gate_bootstrapping_secret_keyset(verify_key);
}
