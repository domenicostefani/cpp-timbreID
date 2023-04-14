#include <gtest/gtest.h>

#define private public
#include "timbreid.h"

TEST(ATTACKTIME, Constructs) {
    EXPECT_NO_THROW(tid::AttackTime<float> attackTime;);
}

// Prepare AttackTime
TEST(ATTACKTIME, Prepares) {
    tid::AttackTime<float> attackTime;
    attackTime.prepare(44100,64);
    // Expect equality.
    EXPECT_EQ(attackTime.sampleRate, 44100);
    EXPECT_EQ(attackTime.blockSize, 64);
}

TEST(BARKSPECBRIGHTNESS, Constructs) {
    EXPECT_NO_THROW(tid::BarkSpecBrightness<float> barkSpecBrightness;);
}

// Prepare BarkSpecBrightness
TEST(BARKSPECBRIGHTNESS, Prepares) {
    tid::BarkSpecBrightness<float> barkSpecBrightness;
    barkSpecBrightness.prepare(44100,64);
    // Expect equality.
    EXPECT_EQ(barkSpecBrightness.sampleRate, 44100);
    EXPECT_EQ(barkSpecBrightness.blockSize, 64);
}

TEST(BARKSPEC, Constructs) {
    EXPECT_NO_THROW(tid::BarkSpec<float> barkSpec;);
}

// Prepare barkSpec
TEST(BARKSPEC, Prepares) {
    tid::BarkSpec<float> barkSpec;
    barkSpec.prepare(44100,64);
    // Expect equality.
    EXPECT_EQ(barkSpec.sampleRate, 44100);
    EXPECT_EQ(barkSpec.blockSize, 64);
}

TEST(BFCC, Constructs) {
    EXPECT_NO_THROW(tid::Bfcc<float> bfcc;);
}

// Prepare Bfcc
TEST(BFCC, Prepares) {
    tid::Bfcc<float> bfcc;
    bfcc.prepare(44100,64);
    // Expect equality.
    EXPECT_EQ(bfcc.sampleRate, 44100);
    EXPECT_EQ(bfcc.blockSize, 64);
}

TEST(CEPSTRUM, Constructs) {
    EXPECT_NO_THROW(tid::Cepstrum<float> cepstrum;);
}

// Prepare Cepstrum
TEST(CEPSTRUM, Prepares) {
    tid::Cepstrum<float> cepstrum;
    cepstrum.prepare(44100,64);
    // Expect equality.
    EXPECT_EQ(cepstrum.sampleRate, 44100);
    EXPECT_EQ(cepstrum.blockSize, 64);
}

TEST(MFCC, Constructs) {
    EXPECT_NO_THROW(tid::Mfcc<float> mfcc;);
}

// Prepare Mfcc
TEST(MFCC, Prepares) {
    tid::Mfcc<float> mfcc;
    mfcc.prepare(44100,64);
    // Expect equality.
    EXPECT_EQ(mfcc.sampleRate, 44100);
    EXPECT_EQ(mfcc.blockSize, 64);
}

TEST(PEAKSAMPLE, Constructs) {
    EXPECT_NO_THROW(tid::PeakSample<float> peakSample;);
}

// Prepare PeakSample
TEST(PEAKSAMPLE, Prepares) {
    tid::PeakSample<float> peakSample;
    peakSample.prepare(44100,64);
    // Expect equality.
    EXPECT_EQ(peakSample.sampleRate, 44100);
    EXPECT_EQ(peakSample.blockSize, 64);
}

TEST(ZEROCROSSING, Constructs) {
    EXPECT_NO_THROW(tid::ZeroCrossing<float> zeroCrossing;);
}

// Prepare ZeroCrossing
TEST(ZEROCROSSING, Prepares) {
    tid::ZeroCrossing<float> zeroCrossing;
    zeroCrossing.prepare(44100,64);
    // Expect equality.
    EXPECT_EQ(zeroCrossing.sampleRate, 44100);
    EXPECT_EQ(zeroCrossing.blockSize, 64);
}



// TEST(KNN, Constructs) {
//     EXPECT_NO_THROW(tid::KNNclassifier knn;);
// }

// ONSET DETECTION
//	-	Aubio::Onset	-	aubioOnsetWrap.h
//	-	Bark	-	bark.hpp

// FEATURE EXTRACTION
//	-	AttackTime	-	attackTime.h
//	-	BarkSpecBrightness	-	barkSpecBrightness.h
//	-	BarkSpec	-	barkSpec.h
//	-	Bfcc	-	bfcc.h
//	-	Cepstrum	-	cepstrum.h
//	-	mfcc.h
//	-	PeakSample	-	peakSample.h
//	-	tidTime.hpp
//	-	timbreid.h
//	-	zeroCrossing.h

// CLASSIFICATION
//	-	KNNclassifier	-	knn.h

// CONVERSION
//	-	Bark2freq	-	bark2freq.h
//	-	Bin2freq	-	bin2freq.h
//	-	Freq2bark	-	freq2bark.h
//	-	Freq2bin	-	freq2bin.h
//	-	Freq2mel	-	freq2mel.h
//	-	mel2freq.h	-	Mel2freq

// UTILS
//	-	*	-	tIDLib.h
//	-	RealTimeLogger	-	tidRTLog.h
//	-	Time	-	tidTime.h
