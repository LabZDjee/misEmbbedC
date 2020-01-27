# miscEmbeddedC
Misc C files of general purpose interest in embedded constrained environments

- **gTimer**: generic soft timer managed from the background and depending on a flag being regularly set (typically from an interrupt) to call the main management method. This method can update a static number of soft timers
- **swUart**: soft UART. Used to implement software UART's which depend on `gTimer` for their timings
- **alphanumCmp**: extended alphanumeric comparison (also taking string length, character case, spaces into account as options)
- **shortIIRLowPassFilter**: implementation of an IIR (Infinite Impulse Response) first order low-pass filter on `short` integers

 