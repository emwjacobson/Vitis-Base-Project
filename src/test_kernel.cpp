#include <tfhe/tfhe.h>
#include <tfhe/tfhe_io.h>
#include <tfhe_gate_bootstrapping_functions.h>
#include <string.h>

extern "C" {
  // // elementary full comparator gate that is used to compare the i-th bit:
  // //   input: ai and bi the i-th bit of a and b
  // //          lsb_carry: the result of the comparison on the lowest bits
  // //   algo: if (a==b) return lsb_carry else return b
  // void compare_bit(LweSample* result, const LweSample* a, const LweSample* b, const LweSample* lsb_carry, LweSample* tmp, const TFheGateBootstrappingCloudKeySet* bk) {
  //   bootsXNOR(tmp, a, b, bk);
  //   bootsMUX(result, tmp, lsb_carry, a, bk);
  // }



  // // this function compares two multibit words, and puts the max in result
  // void test_kernel(LweSample* result, const LweSample* a, const LweSample* b, const int nb_bits, const TFheGateBootstrappingCloudKeySet* bk) {
  // #pragma HLS INTERFACE m_axi port=result
  // #pragma HLS INTERFACE m_axi port=a
  // #pragma HLS INTERFACE m_axi port=b
  // #pragma HLS INTERFACE m_axi port=bk

  //   // LweSample* tmps = new_gate_bootstrapping_ciphertext_array(2, bk->params);

  //   // //initialize the carry to 0
  //   // bootsCONSTANT(&tmps[0], 0, bk);
  //   // //run the elementary comparator gate n times
  //   // for (int i=0; i<nb_bits; i++) {
  //   //     compare_bit(&tmps[0], &a[i], &b[i], &tmps[0], &tmps[1], bk);
  //   // }
  //   // //tmps[0] is the result of the comparaison: 0 if a is larger, 1 if b is larger
  //   // //select the max and copy it to the result
  //   // for (int i=0; i<nb_bits; i++) {
  //   //     bootsMUX(&result[i], &tmps[0], &b[i], &a[i], bk);
  //   // }

  //   // delete_gate_bootstrapping_ciphertext_array(2, tmps);
  //   return;
  // }



  // void test_kernel(LweSample* result, const LweSample* a, const LweSample* b, const TFheGateBootstrappingCloudKeySet* bk) {
  //   // #pragma HLS INTERFACE m_axi port=result
  //   // #pragma HLS INTERFACE m_axi port=a
  //   // #pragma HLS INTERFACE m_axi port=b
  //   // #pragma HLS INTERFACE m_axi port=bk

    // bootsCONSTANT(result, 42, bk);

  //   return;
  // }

  void test_kernel(LweSample_Container* result, const LweSample_Container* a, const LweSample_Container* b, int nb_bits) {
    #pragma HLS INTERFACE m_axi port=result
    #pragma HLS INTERFACE m_axi port=a
    #pragma HLS INTERFACE m_axi port=b

    printf("KERNEL in->b = %i\n", a[0].b);
    printf("KERNEL in->current_variance = %f\n", a[0].current_variance);
    printf("KERNEL in->a[0] = %i\n", a[0].a[0]);

    for(int i=0;i<nb_bits;i++) {
      for(int j=0;j<630;j++) {
        result[i].a[j] = a[i].a[j];
      }
      result[i].b = a[i].b;
      result[i].current_variance = a[i].current_variance;
    }

    // bootsCONSTANT(result, 42, bk);

    return;
  }
}