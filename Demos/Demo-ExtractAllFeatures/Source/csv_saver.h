#include <atomic>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

template <unsigned int FEATURE_NUM> struct Entry
{
    float features[FEATURE_NUM];
    long unsigned int onsetDetectionTime, featureComputationTime;
    int featureExtractionWindowSize, sampleRate, blockSize;
    bool isSpecial = false;
    char message[51];

    void writeMessage(const char message[])
    {
        this->isSpecial = true;
        strncpy(this->message, message, 51);
    }

    void reset()
    {
        for (float &val : features)
            val = 0.0f;
        onsetDetectionTime = 0;
        featureComputationTime = 0;
        isSpecial = false;
        message[0] = '\0';
    }
};

template <unsigned int BUFFER_SIZE, unsigned int FEATURE_NUM, unsigned int PRECISION> class SaveToCsv
{

  public:
    // bool storeEntry(Entry entry) nothrow
    // {
    //     if (tail < BUFFER_SIZE)
    //     {
    //         entries[tail++] = entry;
    //         return true;
    //     }
    //     return false;
    // }

    Entry<FEATURE_NUM> *entryToWrite()
    {
        auto res = tail < BUFFER_SIZE ? &entriesBuffer[tail] : nullptr;
        if (res)
            res->reset();
        return res;
    }

    size_t confirmEntry()
    {
        return ++tail;
    }

    size_t getEntries(Entry<FEATURE_NUM> *entries[])
    {
        entries = &entriesBuffer;
        return tail.load();
    }

    void clearEntries()
    {
        this->tail.store(0);
    }

    void writeToFile(std::string filename, std::vector<std::string> header)
    {
        if (header.size() != FEATURE_NUM)
            throw std::logic_error("Header size must be equal to the number of features (" +
                                   std::to_string(header.size()) + " != " + std::to_string(FEATURE_NUM) + ")");

        std::ofstream csvFile;
        csvFile.open(filename);

        // WRITE HEADER
        csvFile << "onsetDetectionTime,featureComputationTime,featureExtractionWindowSize,sampleRate,blockSize,";
        for (size_t i = 0; i < header.size(); ++i)
        {
            csvFile << header[i];
            if (i != header.size() - 1)
                csvFile << ",";
            else
                csvFile << "\n";
        }

        // WRITE ENTRIES
        int endIndex = tail.load();
        for (size_t i = 0; i < endIndex; ++i)
        {
            csvFile << std::setprecision(std::numeric_limits<double>::digits10 + 1)
                    << entriesBuffer[i].onsetDetectionTime << "," << entriesBuffer[i].featureComputationTime << ","
                    << entriesBuffer[i].featureExtractionWindowSize << "," << entriesBuffer[i].sampleRate << ","
                    << entriesBuffer[i].blockSize << ",";

            if (entriesBuffer[i].isSpecial)
            {
                csvFile << entriesBuffer[i].message << '\n';
            }
            else
            {
                for (size_t j = 0; j < FEATURE_NUM; ++j)
                {
                    csvFile << std::setprecision(PRECISION) << entriesBuffer[i].features[j];
                    if (j != FEATURE_NUM - 1)
                        csvFile << ",";
                    else
                        csvFile << "\n";
                }
            }
        }

        csvFile.close();
    }

  private:
    Entry<FEATURE_NUM> entriesBuffer[BUFFER_SIZE];
    std::atomic<int> tail{0};
};
