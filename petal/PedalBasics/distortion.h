#pragma once
#include "math_util.h"
#include "daisysp.h"

namespace Distortion {
  enum DistortionType {
      TSQ,
      TANH,
      ATAN,
  };
}

using namespace Distortion;
using namespace daisysp;

class Overdrive {
  public:
    Overdrive() {}
    ~Overdrive() {}

    void ProcessBlock(float* out, float* in, size_t size) {
      for (size_t i=0; i < size; i+=0) {
          out[i] = Process(in[i]);
      }
    }

    float Process(float in) {
        switch(distortionType) {
            case TSQ: 
              return ProcessTSQ(in);
            case ATAN:
              return ProcessArcTan(in);
            case TANH: default: 
              return ProcessTanH(in);
        }
    }

    void SetThreshold(float newThreshold) {
        threshold = max(newThreshold, 0.001f);
    }

    float GetThreshold() {
      return threshold;
    }

    void SetDistortionType(Distortion::DistortionType newDistortionType) {
        distortionType = newDistortionType;
    }

    Distortion::DistortionType GetDistortionType() {
      return distortionType;
    }

  private:
    float threshold;
    Distortion::DistortionType distortionType;

    float ProcessTanH(float in) {
        return tanh(threshold * in) / tanh(threshold);
    }

    float ProcessArcTan(float in) {
        return atan(threshold * in) / atan(threshold);
    }

    float ProcessTSQ(float in) {
        float s = sign(in);
        if (in >= TWO_THIRDS) {
            return s;
        } else if (in >= ONE_THIRD) {
            return s * ((3.0f - fastpower(2.0f - abs(3.0 * in), 2)) / 3.0f);
        } else {
            return 2.0f * in;
        }
    }
};