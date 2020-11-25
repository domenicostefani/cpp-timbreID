# cpp-timbreID
An audio feature extraction library for the JUCE framework. Porting of wbrent/timbreID (original Pure Data external) .

!!!**Bare in mind that this is a work in progress**!!!  
I'm new to audio programming and this project has accumulated tecnical debts that I'm gradually fixing.

At the moment logging from the real time thread is not good because it writes to a file (Using the JUCE::filelogger).
The aubioonset wrapper has a funky way of buffering audio blocks in order to account for differences between block size and aubioonset's hop size.
Older demos may have some juce config issues.
Most importantly, the library is now only a set of headers. I need to set up the system to compile it as static or dynamic library.

_Domenico Stefani 20/10/2020_
