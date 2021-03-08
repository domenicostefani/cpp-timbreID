#include <stdexcept>
#include <iostream>
#include <fstream>

template <unsigned int FEATURE_NUM>
struct Entry
{
    float features[FEATURE_NUM];
};

template <unsigned int BUFFER_SIZE, unsigned int FEATURE_NUM, unsigned int PRECISION>
class SaveToCsv{

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

    Entry<FEATURE_NUM>* entryToWrite(){
        return tail < BUFFER_SIZE ? &entriesBuffer[tail] : nullptr;
    }

    size_t confirmEntry(){
        return ++tail;
    }

    size_t getEntries(Entry<FEATURE_NUM>* entries[])
    {
        entries = &entriesBuffer;
        return tail.load();
    }

    void clearEntries() { this->tail.store(0); }

    void writeToFile(std::string filename, std::vector<std::string> header)
    {
        if (header.size() != FEATURE_NUM)
            throw std::logic_error("Header size must be equal to the number of features");

        std::ofstream csvFile;
        csvFile.open (filename);
        // WRITE HEADER
        for (size_t i=0; i < header.size(); ++i)
        {
            csvFile << header[i];
            if( i != header.size()-1)
                csvFile  << ",";
            else
                csvFile  << "\n";
        }
        // WRITE ENTRIES
        int endIndex = tail.load();
        for (size_t i=0; i < endIndex; ++i)
        {
            for (size_t j=0; j < FEATURE_NUM; ++j)
            {
                csvFile << std::setprecision(PRECISION) << entriesBuffer[i].features[j];
                if( j != FEATURE_NUM-1)
                    csvFile  << ",";
                else
                    csvFile  << "\n";
            }
        }

        csvFile.close();
    }

private:
    Entry<FEATURE_NUM> entriesBuffer[BUFFER_SIZE];
    std::atomic<int> tail{0};
};
