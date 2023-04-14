
namespace tid   /* TimbreID namespace*/
{
class Bin2freq
{
public:
    static float calculate(float bin);
    static void setWinSampRate(long int windowSize, double sampleRate);
    static unsigned long int getWindowSize();
    static unsigned long int getSampleRate();
private:
    static unsigned long int windowSize;
    static double sampleRate;
};
} // namespace tid
