// Host-side parity test: C++ features + inference against the Python values in selftest.h.
// Build: clang++ -std=c++17 -O2 -I firmware/classifier -o /tmp/clf_test tests/host/classifier_selftest.cpp

#include <cstdio>
#include <cmath>
#include "classifier.h"
#include "selftest.h"
int main() {
  float feats[CLF_N_FEATURES];
  clf_features(SELFTEST_RAW, feats);
  ClfResult r = clf_infer(feats);
  float mf = 0, mp = 0;
  for (int i = 0; i < CLF_N_FEATURES; i++) {
    float sc = fabsf(SELFTEST_FEATURES[i]) > 1 ? fabsf(SELFTEST_FEATURES[i]) : 1;
    float e = fabsf(feats[i] - SELFTEST_FEATURES[i]) / sc; if (e > mf) mf = e;
    if (e > 1e-3f) printf("feature %d: c=%f py=%f\n", i, feats[i], SELFTEST_FEATURES[i]);
  }
  for (int j = 0; j < CLF_N_CLASSES; j++) { float e = fabsf(r.probs[j] - SELFTEST_PROBS[j]); if (e > mp) mp = e; }
  printf("label=%s conf=%.3f probs=%.4f %.4f %.4f  py=%.4f %.4f %.4f\nmax_feat_rel_err=%.2e max_prob_err=%.2e %s\n",
         CLF_CLASSES[r.label], r.conf, r.probs[0], r.probs[1], r.probs[2], SELFTEST_PROBS[0], SELFTEST_PROBS[1], SELFTEST_PROBS[2],
         mf, mp, (mf < 1e-3 && mp < 1e-3) ? "PASS" : "FAIL");
}
