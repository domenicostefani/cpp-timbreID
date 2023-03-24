#pragma once


#include <regex>
#include <set>
#include <sstream>

namespace JsonConf
{

inline juce::var parseJson(std::string configfilepath)
{
    // Read file
    juce::File fileToRead = juce::File(configfilepath);
    if (! fileToRead.existsAsFile())
        throw std::logic_error("Configuration file does not exist at " + configfilepath);
    // Read content as string
    auto configJsonString = fileToRead.loadFileAsString();
    // Parse JSON
    juce::var parsedJson;
    if( juce::JSON::parse(configJsonString, parsedJson).wasOk() )
        return parsedJson;
    throw std::logic_error("Error parsing the json file at '"+configfilepath+"')'");
}

inline std::string getNestedStringProperty(juce::var parsedJson, std::string propertyName1, std::string propertyName2)
{
    std::string readProperty = parsedJson[propertyName1.c_str()][propertyName2.c_str()].toString().toStdString();
    if (readProperty == "")
        throw std::logic_error(propertyName2 + " property missing in the configuration file");
    else
        return readProperty;
}


inline std::vector<std::string> getNestedStringVectorProperty(juce::var parsedJson, std::string propertyName1, std::string propertyName2)
{
    juce::Array<juce::var>* readProperty = parsedJson[propertyName1.c_str()][propertyName2.c_str()].getArray();
    if (readProperty->size() == 0)
        throw std::logic_error(propertyName2 + " property missing in the configuration file");

    std::vector<std::string> res;
    for (auto item : *readProperty)
        res.push_back(item.toString().toStdString());
    
    return res;
}



inline std::vector<float> getNestedFloatSciNotationVectorProperty(juce::var parsedJson, std::string propertyName1, std::string propertyName2)
{
    std::vector<std::string> readProperty = getNestedStringVectorProperty(parsedJson, propertyName1, propertyName2);
    std::vector<float> res;

    for (const std::string& item : readProperty) {
        // Read scientific notation string and convert to float
        std::istringstream iss(item);
        float value;
        iss >> value;
        res.push_back(value);
    }

    return res;
}

} //namespace JsonConf
