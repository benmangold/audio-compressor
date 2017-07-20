# benCompressor~ PureData External #

This is a PureData external, implementing an audio compression algorithm.

The algorithm is implemented in C, and found in benCompressor~.c

Compression serves to limit the dynamic range of a signal.  It lowers signal amplitude upon reaching a certain amplitude threshold.  This allows for the entire signal to be turned up in amplitude.

Attack/Release envelope controls allow the timing of the algorithm to be adjusted.

## Compiling the External 

Navigate to project directory:
> $ cd benCompressor~

Run 'make' command to generate .pd_darwin file:
> $ make

## Using the External

Usage is demonstrated in benCompressor~-help.pd

Download PureData Vanilla (Pd is required to open the help patch)
https://puredata.info/downloads/pure-data

"Pd (aka Pure Data) is a real-time graphical programming environment for audio, video, and graphical processing."