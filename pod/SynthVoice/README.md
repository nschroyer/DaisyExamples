# Description
Simple Synth voice with resonant filter, self cycling envelope, and vibrato control.

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Led | Mode Indicate | 1. Blue, 2. Green, 3. Red |
| Turn Encoder | Mode Select | |
| Press Encoder | Waveform Select | |
| Button 1 | Trigger envelope | |
| Button 2 | Envelope Cycle | Led 2 lights purple when cycling |

| Control| Mode 1: Filter / Freq | Mode 2: Envelope | Mode 3: Vibrato |
| --- | --- | --- | --- |
| LED | Blue | Green | Red |
| Knob 1 | Cutoff | Attack | Rate |
| Knob 2 | Osc. Freq. | Decay | Depth |




# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/pod/SynthVoice/resources/SynthVoice.png" alt="Button_schem.png" style="width: 100%;"/>

# Code Snippet  
```cpp  
//Process Samples
float ad_out = ad.Process();
vibrato = lfo.Process();

osc.SetFreq(oscFreq + vibrato);

sig = osc.Process();
sig = flt.Process(sig);
sig *= ad_out;
```
    
