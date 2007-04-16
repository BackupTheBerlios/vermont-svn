#include "cusum-test.h"

double cusum(const int & X, const double & beta, double & g) {

  // update the cumulative sum
  g = g + (X - beta);

  // clamp it to zero if negative
  if ( g >= 0.0)
    return g;
  else
    return 0.0;
}
