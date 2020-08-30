#include "daisy_petal.h"
#include "daisysp.h" 
#include "math_util.h"
#include "distortion.h"

#define MAX_DELAY static_cast<size_t>(48000 * 1.f)

using namespace daisy;
using namespace daisysp;

// Declare a local daisy_petal for hardware access
static DaisyPetal hardware;

// Reverb
static Parameter reverbTime, reverbCrossfadeAmount;
static bool bypassReverb;
static ReverbSc reverb;
static CrossFade reverbCrossfadeL, reverbCrossfadeR;

// Pitch Shifter
static int32_t pitchShiftAmount;
const int32_t maxPitchShiftOctaves = 1;
const int32_t maxStepsUp = 12 * maxPitchShiftOctaves;
const int32_t maxStepsDown = -maxStepsUp;
static PitchShifter DSY_SDRAM_BSS pitchShifter;
static CrossFade pitchCrossfadeL, pitchCrossfadeR;
static Parameter pitchCrossfadeAmount;


// Overdrive
static Parameter overdriveThreshold;
static bool bypassOverdrive;
static Overdrive overdrive;

static float maxSample = 0.0f;
static float maxSampleCoeff = 1.0f / (1.0f * DSY_AUDIO_SAMPLE_RATE);
static bool hasError;

void getReverbSample(float &outl, float &outr, float inl, float inr) {
    reverb.Process(inl, inr, &outl, &outr);
    if (bypassReverb) {
        outl = inl;
        outr = inr;
    } else {
        outl = reverbCrossfadeL.Process(inl, outr);
        outr = reverbCrossfadeR.Process(inr, outr);
    }
}

void getPitchShifterSample(float &outl, float &outr, float inl, float inr) {
    pitchShifter.SetTransposition((float) pitchShiftAmount);
    outl = pitchShifter.Process(inl);
    outr = pitchShifter.Process(inr);
    outl = pitchCrossfadeL.Process(inl, outl);
    outr = pitchCrossfadeR.Process(inr, outr);
}

void updateControls() {
    hardware.DebounceControls();   

    // Reverb Controls
    if (hardware.switches[DaisyPetal::SW_1].RisingEdge()) {
        bypassReverb = !bypassReverb;
    }
    reverb.SetFeedback(reverbTime.Process());
    reverbCrossfadeAmount.Process();
    reverbCrossfadeL.SetPos(reverbCrossfadeAmount.Value());
    reverbCrossfadeR.SetPos(reverbCrossfadeAmount.Value());

    // Pitch Shift
    if (hardware.encoder.RisingEdge()) {
        pitchShiftAmount = 0;
    }
    pitchShiftAmount += hardware.encoder.Increment();
    pitchShiftAmount = clamp(pitchShiftAmount, maxStepsDown, maxStepsUp);
    pitchCrossfadeAmount.Process();
    pitchCrossfadeL.SetPos(pitchCrossfadeAmount.Value());
    pitchCrossfadeR.SetPos(pitchCrossfadeAmount.Value());

    // Overdrive
    if (hardware.switches[DaisyPetal::SW_2].RisingEdge()) {
        bypassOverdrive = !bypassOverdrive;
    }
    overdrive.SetThreshold(overdriveThreshold.Process());
    overdrive.SetDistortionType(hardware.switches[DaisyPetal::SW_6].Pressed() ? Distortion::ATAN : Distortion::TANH);
}

// This runs at a fixed rate, to prepare audio samples
void callback(float *in, float *out, size_t size) {
    hasError = false;
    float outl, outr, inl, inr;

    updateControls();

    for (size_t i=0; i < size; i++) {
        fonepole(maxSample, max(maxSample, abs(in[i])), maxSampleCoeff);
    }

    if (maxSample > 0.0f) {
        for (size_t i=0; i < size; i++) {
            in[i] = in[i] / maxSample;
        }

        if (!bypassOverdrive) {
            for (size_t i=0; i < size; i++) {
                in[i] = overdrive.Process(in[i]);
            }
        }

        for (size_t i=0; i < size; i++) {
            in[i] = in[i] * maxSample;
        }
    } else {
        hasError = true;
    }

    for (size_t i = 0; i < size; i += 2) {
        inl = in[i];
        inr = in[i+1];

        getPitchShifterSample(outl, outr, inl, inr);
        getReverbSample(outl, outr, outl, outr);

        out[i] = outl;
        out[i+1] = outr;
    }
}

void initReverb(float samplerate) {
    reverbTime.Init(hardware.knob[hardware.KNOB_1], 0.6f, 0.999f, Parameter::LOGARITHMIC);
    reverbCrossfadeAmount.Init(hardware.knob[hardware.KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
    reverb.Init(samplerate);
    reverb.SetLpFreq(samplerate / 2.0f);
    reverbCrossfadeL.Init(CROSSFADE_CPOW);
    reverbCrossfadeR.Init(CROSSFADE_CPOW);
}

void initPitchShifter(float samplerate) {
    pitchCrossfadeAmount.Init(hardware.knob[hardware.KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);
    pitchShifter.Init(samplerate);
    pitchCrossfadeL.Init(CROSSFADE_CPOW);
    pitchCrossfadeR.Init(CROSSFADE_CPOW);
}

void initOverdrive() {
    overdriveThreshold.Init(hardware.knob[hardware.KNOB_4], 1.0f, 50.0f, Parameter::LINEAR);
}

void updateLEDs() {
    hardware.ClearLeds();
    hardware.SetFootswitchLed(hardware.FOOTSWITCH_LED_1, bypassReverb ? 0.0f : 1.0f);
    hardware.SetFootswitchLed(hardware.FOOTSWITCH_LED_2, bypassOverdrive ? 0.0f : overdrive.GetDistortionType() == Distortion::TANH ? 0.5f : 1.0f);

    for (int32_t led = 0; led < 8; led++) {
        if (mod(pitchShiftAmount, 8) == led) {
            float r,g,b;
            r = g = b = 0.0f;
            if (pitchShiftAmount > 0) {
                g = (float) pitchShiftAmount / 12 / 3;
            }
            if (pitchShiftAmount < 0) {
                r = (float) abs(pitchShiftAmount) / 12 / 3;
            }
            hardware.SetRingLed(static_cast<DaisyPetal::RingLed>(led), r, g, b);
        }
    }

    if (hasError) {
        hardware.ClearLeds();
        for (int32_t led = 0; led < 8; led++) {
            hardware.SetRingLed(static_cast<DaisyPetal::RingLed>(led), 1.0f, 0.0f, 0.0f);
        }
    }

    hardware.UpdateLeds();
}

int main(void)
{
    float samplerate;

    hardware.Init();
    samplerate = hardware.AudioSampleRate();
    maxSampleCoeff = 1.0f / (1.0f * samplerate);

    initOverdrive();
    initReverb(samplerate);
    initPitchShifter(samplerate);

    hardware.StartAdc();
    hardware.StartAudio(callback);
    while(1) 
    {
        // Do Stuff Infinitely Here
        dsy_system_delay(10);
        updateLEDs();
    }
}
