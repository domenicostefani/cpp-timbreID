/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

namespace BinaryData
{

//================== TODO.txt ==================
static const unsigned char temp_binary_data_0[] =
"Instructions for the programmer, should be removed before release, disregard this file if found\n"
"\n"
"+------------------------------------------------------------------------------+\n"
"|                      CODING INSTRUCTIONS AND TODO LISTS                      |\n"
"+------------------------------------------------------------------------------+\n"
"\n"
"PROGRESS MADE SO FAR:\n"
"+----------------------+-------------------------+-------+------------+--------+\n"
"|        MODULE        |  Most relevant measures | Coded | Demo Coded | Tested |\n"
"+----------------------+-------------------------+-------+------------+--------+\n"
"| attackTime~.c ------------------------------------ X ------- X\n"
"| bark~.c ------------------------------------------ X ------- X\n"
"| barkSpecBrightness~.c ---------------------------- X ------- X\n"
"| barkSpec~.c ----------------------- X ------------ X ------- X\n"
"| barkSpecCentroid~.c\n"
"| barkSpecFlatness~.c\n"
"| barkSpecFlux~.c\n"
"| barkSpecIrregularity~.c\n"
"| barkSpecKurtosis~.c\n"
"| barkSpecRolloff~.c\n"
"| barkSpecSkewness~.c\n"
"| barkSpecSlope~.c\n"
"| barkSpecSpread~.c\n"
"| bfcc~.c --------------------------- X ------------ X ------- X\n"
"| cepstrum~.c ----------------------- X ------------ X ------- X\n"
"| cepstrumPitch~.c\n"
"| chroma~.c\n"
"| dct~.c\n"
"| magSpec~.c\n"
"| maxSample~.c\n"
"| maxSampleDelta~.c\n"
"| melSpec~.c\n"
"| mfcc~.c --------------------------- X ------------ X ------- X\n"
"| minSample~.c\n"
"| minSampleDelta~.c\n"
"| peakSample~.c  ----------------------------------- X ------- X\n"
"| phaseSpec~.c\n"
"| sampleBuffer~.c\n"
"| specBrightness~.c\n"
"| specCentroid~.c\n"
"| specFlatness~.c\n"
"| specFlux~.c\n"
"| specHarmonicity~.c\n"
"| specIrregularity~.c\n"
"| specKurtosis~.c\n"
"| specRolloff~.c\n"
"| specSkewness~.c\n"
"| specSlope~.c\n"
"| specSpread~.c\n"
"| tempo~.c\n"
"| tID_fft~.c\n"
"| waveDirChange~.c\n"
"| waveSlope~.c\n"
"| zeroCrossing~.c ---------------------------------- X ------- X\n"
"+----------------------+-------------------------+-------+------------+--------+\n"
"\n"
"\n"
"INSTRUCTIONS:\n"
"\n"
"+------------------------------------------------------------------------------+\n"
"| INSTRUCTIONS FOR MODULE PORTING (TODO LIST):                                 |\n"
"+------------------------------------------------------------------------------+\n"
" - Replace HARD tabs with SOFT tabs\n"
" - Copy licence disclaimer\n"
" - Correct DATE and module NAME in licence disclaimer\n"
" - Create constuctors\n"
" - Copy move constuctor + destructor\n"
" - Copy Prepare & Reset methods\n"
" - Copy substitutions for filterAvg, specBand, powerSpectrum ***\n"
" - Comment every method in Doxygen style\n"
" - Add tIDLib namespace in front of constants\n"
" - look for unsolved TODOs\n"
"\n"
"+------------------------------------------------------------------------------+\n"
"| INSTRUCTIONS FOR REPLACEMENT OF OLD BOOL FLAGS                               |\n"
"+------------------------------------------------------------------------------+\n"
"    filterAvg       -->         is replaced by       -->        filterOperation\n"
"        (false) --> (tIDLib::FilterOperation::sumFilterEnergy)\n"
"        (true)  --> (tIDLib::FilterOperation::averageFilterEnergy)\n"
"\n"
"    specBandAvg     -->         is replaced by       -->        filterState\n"
"        (false) --> (tIDLib::FilterState::filterEnabled)\n"
"        (true)  --> (tIDLib::FilterState::filterDisabled)\n"
"\n"
"    powerSpectrum   -->         is replaced by       -->        spectrumTypeUsed\n"
"        (false) --> (tIDLib::SpectrumType::magnitudeSpectrum)\n"
"        (true)  --> (tIDLib::SpectrumType::powerSpectrum)\n"
"\n"
"\n"
"+------------------------------------------------------------------------------+\n"
"| TODO: (Technical Debt)                                                       |\n"
"+------------------------------------------------------------------------------+\n"
" - Some modules have the perform() function and some other storeAudioBlock,\n"
"   choose one name\n"
"\n"
"\n"
"REMOVE \"=== bark module initialization ===\" from old demos\n";

const char* TODO_txt = (const char*) temp_binary_data_0;


const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)
{
    unsigned int hash = 0;

    if (resourceNameUTF8 != nullptr)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0xe17ebef7:  numBytes = 3784; return TODO_txt;
        default: break;
    }

    numBytes = 0;
    return nullptr;
}

const char* namedResourceList[] =
{
    "TODO_txt"
};

const char* originalFilenames[] =
{
    "TODO.txt"
};

const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8)
{
    for (unsigned int i = 0; i < (sizeof (namedResourceList) / sizeof (namedResourceList[0])); ++i)
    {
        if (namedResourceList[i] == resourceNameUTF8)
            return originalFilenames[i];
    }

    return nullptr;
}

}
