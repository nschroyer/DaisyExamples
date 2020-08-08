#include "daisy_petal.h"
#include "daisysp.h" 

#define MAX_DELAY static_cast<size_t>(48000 * 1.f)

using namespace daisy;
using namespace daisysp;

// Declare a local daisy_petal for hardware access
static DaisyPetal hardware;

// Gain
static Parameter gain;

// Reverb
static Parameter reverbTime, reverbSend;
static bool bypassReverb, dryMixReverb;
static ReverbSc reverb;

// Delay
static Parameter delayFeedback, delayTime;
static bool bypassDelay;
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delayL;
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delayR;
static float currentDelay;

// Pitch Shifter
static int32_t pitchShiftAmount;
const int32_t maxPitchShiftOctaves = 1;
const int32_t maxStepsUp = 12 * maxPitchShiftOctaves;
const int32_t maxStepsDown = -maxStepsUp;
static PitchShifter DSY_SDRAM_BSS pitchShifter;

void getGainSample(float &outl, float &outr, float inl, float inr) {
    outl = inl * gain.Value();
    outr = inr * gain.Value();
}

void getReverbSample(float &outl, float &outr, float inl, float inr) {
    float send = reverbSend.Value();
    float dryMix = 1 - (send * !dryMixReverb);
    reverb.Process(inl, inr, &outl, &outr);
    if (bypassReverb) {
        outl = inl;
        outr = inr;
    } else {
        outl = send * outl + dryMix * inl;
        outr = send * outr + dryMix * inr;
    }
}

void getDelaySample(float &outl, float &outr, float inl, float inr) {
    fonepole(currentDelay, delayTime.Value(), .00007f);
    delayL.SetDelay(delayTime.Value());
    delayR.SetDelay(delayTime.Value());
    if (bypassDelay) {
        outl = inl;
        outr = inr;
    } else {
        outl = delayL.Read();
        outr = delayR.Read();

        float feedback = delayFeedback.Value();
        delayL.Write((feedback * outl) + inl);
        outl = (feedback * outl) + inl;

        delayR.Write((delayFeedback.Value() * outr) + inr);
        outr = (feedback * outr) + inr;
    }
}

void getPitchShifterSample(float &outl, float &outr, float inl, float inr) {
    pitchShifter.SetTransposition((float) pitchShiftAmount);
    outl = pitchShifter.Process(inl);
    outr = pitchShifter.Process(inr);
}

int max(int a, int b) {
    if (a >= b) {
        return a;
    } else {
        return b;
    }
}

int min(int a, int b) {
    if (a <= b) {
        return a;
    } else {
        return b;
    }
}

int clamp(int value, int minimum, int maximum) {
    return max(minimum, min(maximum, value));
}
int mod(int a, int b) { return (a%b+b)%b; }

void updateControls() {
    hardware.DebounceControls();   

    // Gain Controls
    gain.Process();

    // Reverb Controls
    if (hardware.switches[DaisyPetal::SW_1].RisingEdge()) {
        bypassReverb = !bypassReverb;
    }
    dryMixReverb = hardware.switches[DaisyPetal::SW_5].Pressed();
    reverb.SetFeedback(reverbTime.Process());
    reverbSend.Process();

    // Delay Controls
    if (hardware.switches[DaisyPetal::SW_2].RisingEdge()) {
        bypassDelay = !bypassDelay;
        if (!bypassDelay) {
            delayL.Reset();
            delayR.Reset();
        }
    }
    delayFeedback.Process();
    delayTime.Process();

    // Pitch Shift
    if (hardware.encoder.RisingEdge()) {
        pitchShiftAmount = 0;
    }
    pitchShiftAmount += hardware.encoder.Increment();
    pitchShiftAmount = clamp(pitchShiftAmount, maxStepsDown, maxStepsUp);
}

// This runs at a fixed rate, to prepare audio samples
void callback(float *in, float *out, size_t size) {
    float outl, outr, inl, inr;

    updateControls();
    for (size_t i = 0; i < size; i += 2) {
        inl = in[i];
        inr = in[i+1];

        getGainSample(outl, outr, inl, inr);
        getPitchShifterSample(outl, outr, outl, outr);
        getReverbSample(outl, outr, outl, outr);
        getDelaySample(outl, outr, outl, outr);

        out[i] = outl;
        out[i+1] = outr;
    }
}

void initReverb(float samplerate) {
    reverbTime.Init(hardware.knob[hardware.KNOB_1], 0.6f, 0.999f, Parameter::LOGARITHMIC);
    reverbSend.Init(hardware.knob[hardware.KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
    reverb.Init(samplerate);
    reverb.SetLpFreq(samplerate / 2.0f);
}

void initDelay(float samplerate) {
    delayFeedback.Init(hardware.knob[hardware.KNOB_4], 0.0f, 1.0f, Parameter::LINEAR);
    delayTime.Init(hardware.knob[hardware.KNOB_2], samplerate * 0.05f, MAX_DELAY, Parameter::LOGARITHMIC);
    currentDelay = delayTime.Process();
    delayL.Init();
    delayR.Init();
}

void initGain() {
    gain.Init(hardware.knob[hardware.KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);
}

void initPitchShifter(float samplerate) {
    pitchShifter.Init(samplerate);
}

void updateLEDs() {
    hardware.ClearLeds();
    hardware.SetFootswitchLed(hardware.FOOTSWITCH_LED_1, bypassReverb ? 0.0f : 1.0f);
    hardware.SetFootswitchLed(hardware.FOOTSWITCH_LED_2, bypassDelay ? 0.0f : 1.0f);
    hardware.SetFootswitchLed(hardware.FOOTSWITCH_LED_4, gain.Value());

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

    hardware.UpdateLeds();
}

int main(void)
{
    float samplerate;

    hardware.Init();
    samplerate = hardware.AudioSampleRate();

    initGain();
    initReverb(samplerate);
    initDelay(samplerate);
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
