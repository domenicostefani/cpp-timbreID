#pragma once


constexpr unsigned long int WINDOWSIZEDEFAULT = 1024;
constexpr unsigned long int  SAMPLERATEDEFAULT = 44100;
constexpr float MAXBARKS = 26.0;
constexpr float MAXBARKFREQ = 22855.4;
constexpr float MAXMELFREQ = 22843.6;
constexpr float MAXMELS = 3962.0;



enum t_bark2freqFormula
{
	bark2freqFormula0 = 0,
	bark2freqFormula1,
	bark2freqFormula2
};

enum t_freq2barkFormula
{
	freq2barkFormula0 = 0,
	freq2barkFormula1,
	freq2barkFormula2
};



/* ---------------- conversion functions ---------------------- */
float tIDLib_freq2bin(float freq, float n, float sr);
float tIDLib_bin2freq(float bin, float n, float sr);
float tIDLib_freq2bark(float freq);
float tIDLib_bark2freq(float bark);
float tIDLib_freq2mel(float freq);
float tIDLib_mel2freq(float mel);
/* ---------------- END conversion functions ---------------------- */
