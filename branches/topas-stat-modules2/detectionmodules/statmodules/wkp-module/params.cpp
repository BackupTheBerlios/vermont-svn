#include "params.h"

void Params::init(int size) {

    cov = gsl_matrix_calloc (size, size);
    evec = gsl_matrix_calloc (size, size);
    // initialize sumsOfMetrics and sumsOfProducts
    for (int i = 0; i < size; i++) {
      sumsOfMetrics.push_back(0);
      std::vector<long long int> v;
      sumsOfProducts.push_back(v);
      for (int j = 0; j < size; j++)
        sumsOfProducts.at(i).push_back(0);
    }

    learning_phase_nr_for_pca = 0;
    pca_ready = false;

    return;

  }
