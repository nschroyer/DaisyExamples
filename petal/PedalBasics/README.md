# PedalBasics

A basic pedal board with reverb and delay. More functions coming soon.

## Reverb


| Control | Hardware |
| - | - |
| Bypass Effect | Switch 1 |
| Room Size | Knob 1 |
| Dry/Wet Crossfade | Knob 3 |

## Distortion

| Control | Hardware |
| - | - |
| Bypass Effect | Switch 2 |
| Algorithm (Up=tanh, Down=arctan) | Switch 6 |
| Drive Amount | Knob 4 |

## Pitch Shifter
Pitch shift up or down an octave.
This is a little buggy right now. Pitch shifting up works fine, but shifting down finicky.

| Control | Hardware |
| - | - |
| Shift Semitone Up/Down | Encoder (turn dial) |
| Reset | Encoder (push in) |
| Dry/Wet Crossfade | Knob 2 |

## Install
Build: `make`
Install:
* Terminal - `make program-dfu`
* Online - https://electro-smith.github.io/Programmer/
