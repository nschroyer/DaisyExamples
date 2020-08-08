#include "daisy_petal.h"
#include "daisysp.h" 

#define MAX_DELAY static_cast<size_t>(48000 * 1.f)

using namespace daisy;
using namespace daisysp;

// Declare a local daisy_petal for hardware access
static DaisyPetal hardware;

// Reverb
static Parameter reverbTime, reverbSend;
static bool bypassReverb;
static ReverbSc reverb;

// Delay
static Parameter delayFeedback, delayTime;
static bool bypassDelay;
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delayL;
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delayR;

void getReverbSample(float &outl, float &outr, float inl, float inr) {
    float send = reverbSend.Value();
    reverb.Process(inl, inr, &outl, &outr);
    if (bypassReverb) {
        outl = inl;
        outr = inr;
    } else {
        outl = send * outl + (1 - send) * inl;
        outr = send * outr + (1 - send) * inr;
    }
}

void getDelaySample(float &outl, float &outr, float inl, float inr) {
    if (bypassDelay) {
        outl = inl;
        outr = inr;
    } else {
        outl = delayL.Read();
        outr = delayR.Read();

        float feedback = delayFeedback.Value();
        delayL.Write((feedback * outl) + inl);
        outl = (feedback * outl) + ((1.0f - feedback) * inl);

        delayR.Write((delayFeedback.Value() * outr) + inr);
        outr = (feedback * outr) + ((1.0f - feedback) * inr);
    }
}

void updateControls() {
    hardware.DebounceControls();   
    if (hardware.switches[DaisyPetal::SW_1].RisingEdge()) {
        bypassReverb = !bypassReverb;
    }
    reverb.SetFeedback(reverbTime.Process());
    reverbSend.Process();

    if (hardware.switches[DaisyPetal::SW_2].RisingEdge()) {
        bypassDelay = !bypassDelay;
    }
    delayFeedback.Process();
    delayTime.Process();
    delayL.SetDelay(delayTime.Value());
    delayR.SetDelay(delayTime.Value());
}

// This runs at a fixed rate, to prepare audio samples
void callback(float *in, float *out, size_t size) {
    float outl, outr, inl, inr;

    updateControls();
    for (size_t i = 0; i < size; i += 2) {
        inl = in[i];
        inr = in[i+1];
        getReverbSample(outl, outr, inl, inr);
        getDelaySample(outl, outr, outl, outr);

        out[i] = outl;
        out[i+1] = outr;
    }
}

void updateLEDs() {
    hardware.ClearLeds();
    hardware.SetFootswitchLed(hardware.FOOTSWITCH_LED_1, bypassReverb ? 0.0f : 1.0f);
    hardware.SetFootswitchLed(hardware.FOOTSWITCH_LED_2, bypassDelay ? 0.0f : 1.0f);
    hardware.UpdateLeds();
}

void initReverb(float samplerate) {
    reverbTime.Init(hardware.knob[hardware.KNOB_1], 0.6f, 0.999f, Parameter::LOGARITHMIC);
    reverbSend.Init(hardware.knob[hardware.KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
    reverb.Init(samplerate);
    reverb.SetLpFreq(samplerate / 2.0f);
}

void initDelay(float samplerate) {
    delayFeedback.Init(hardware.knob[hardware.KNOB_4], 0.0f, 1.0f, Parameter::LOGARITHMIC);
    delayTime.Init(hardware.knob[hardware.KNOB_2], samplerate * 0.5f, MAX_DELAY, Parameter::LOGARITHMIC);
    delayL.Init();
    delayR.Init();
}

int main(void)
{
    float samplerate;

    hardware.Init();
    samplerate = hardware.AudioSampleRate();

    initReverb(samplerate);
    initDelay(samplerate);

    hardware.StartAdc();
    hardware.StartAudio(callback);
    while(1) 
    {
        // Do Stuff Infinitely Here
        dsy_system_delay(10);
        updateLEDs();
    }
}
