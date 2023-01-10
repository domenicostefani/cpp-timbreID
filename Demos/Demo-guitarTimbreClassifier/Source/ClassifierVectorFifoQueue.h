/*
  ===================================================================================
     Lockfree Fifo queue to feed features to the Classifier and receive predictions

     Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
     Date: 13th May 2021
  ===================================================================================
*/
#pragma once

#include <array>    // Array is used for the queue buffer
#include <vector>   // Vector is used for the inner elements

template<std::size_t BUFFER_SIZE>
class ClassifierVectorQueue
{
public:
    using ElementType = std::vector<float>;
    /** Construct a ClassifierVectorQueue that can store float vectors
    */
    ClassifierVectorQueue() {
    }

    void reserveVectorSize (size_t elementSize)
    {
        for(int i=0;i<BUFFER_SIZE;++i)
        {
            data[i].resize(elementSize); // Length of the elements in the lock free queue
            juce::FloatVectorOperations::clear(&(data[i][0]),elementSize);
        }
    }

    /**
     * @brief
     * Adapted from JUCE example at https://docs.juce.com/master/classAbstractFifo.html#details
    */
    void writeMany (const ElementType* items, int numItems)
    {
        int start1, size1, start2, size2;
        lockFreeFifo.prepareToWrite (numItems, start1, size1, start2, size2);

        if (size1 > 0)
            for(int i=0;i<size1;++i)
                data[start1+i] = items[i];

        if (size2 > 0)
            for(int i=0;i<size2;++i)
                data[start2+i] = items[size1+i];

        lockFreeFifo.finishedWrite (size1 + size2);
    }

    /**
     * Adapted from JUCE example at https://docs.juce.com/master/classAbstractFifo.html#details
    */
    void readMany (ElementType* items, int numItems)
    {
        int start1, size1, start2, size2;
        lockFreeFifo.prepareToRead (numItems, start1, size1, start2, size2);

        if (size1 > 0)
            for(int i=0;i<size1;++i)
                items[i] = data[start1+i];

        if (size2 > 0)
            for(int i=0;i<size2;++i)
                items[size1+i] = data[start2+i];

        lockFreeFifo.finishedRead (size1 + size2);
    }

    void write (const ElementType& element) {
        writeMany(&element,1);
    }
    bool read (ElementType& element) {
        if (lockFreeFifo.getNumReady() <= 0)
            return false;
        readMany(&element,1);
        return true;
    }

private:
    std::array<ElementType,BUFFER_SIZE> data;   // The data structure that contains the elements of the queue
    AbstractFifo lockFreeFifo { BUFFER_SIZE };  // Lock-free queue management object
};