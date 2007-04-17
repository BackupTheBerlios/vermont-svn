#include "cusum-test.h"

// update the cumulative sum g
double cusum(const int & X, const double & beta, double & g) {

  // we need tmp because g is a reference parameter and
  // we wont update it, if it is < 0
  double tmp = g + (X - beta);

  // clamp g to zero if negative
  if ( tmp >= 0.0)
    g = tmp;
  else
    g = 0.0;

  return g;
}
