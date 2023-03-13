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




enum feature_extractor_t {
    NONE = 0,
    ATTACK_TIME,
    BARK_SPEC_BRIGHTNESS,
    BARK_SPEC,
    BFCC,
    MFCC,
    CEPSTRUM,
    PEAK_SAMPLE,
    ZERO_CROSSING,
};

const std::map<feature_extractor_t,std::string> stringPrefixes = {
    {feature_extractor_t::ATTACK_TIME, "attackTime"},
    {feature_extractor_t::BARK_SPEC_BRIGHTNESS, "barkSpecBrightness"},
    {feature_extractor_t::BARK_SPEC, "barkSpec"},
    {feature_extractor_t::BFCC, "bfcc"},
    {feature_extractor_t::MFCC, "mfcc"},
    {feature_extractor_t::CEPSTRUM, "cepstrum"},
    {feature_extractor_t::PEAK_SAMPLE, "peakSample"},
    {feature_extractor_t::ZERO_CROSSING, "zeroCrossing"}
};

class FeatureParser
{
public:
    struct Feature{
        std::string name;
        feature_extractor_t extractorType;
        Feature(std::string featureName, feature_extractor_t extractorType) : name(name), extractorType(extractorType) { }
        virtual ~Feature() = default;
    };
    struct VectorFeature : public Feature{
        size_t index;
        VectorFeature(std::string featureName, feature_extractor_t extractorType, size_t index) : Feature(name,extractorType), index(index) {}
    };
    struct NamedFeature : public Feature{
        std::string valuename;
        NamedFeature(std::string featureName, feature_extractor_t extractorType, std::string valuename) : Feature(name,extractorType), valuename(valuename) {}
    };
    
    inline feature_extractor_t getExtractorFromPrefix(std::string prefix) const
    {
        feature_extractor_t current_extractor = feature_extractor_t::NONE;
        for (const auto& [kExtractor, vPrefix] : stringPrefixes)
            if (prefix == vPrefix)
                current_extractor = kExtractor;
        if (current_extractor == feature_extractor_t::NONE)
            throw std::logic_error(prefix + " name is not correctly written, or it does not come from a known feature extractor");
        return current_extractor;
    }

    std::vector<Feature*> featureList;
    std::set<feature_extractor_t> precomputationList;

    

    FeatureParser (std::vector<std::string> readFeatures)
    {
        this->featureList.clear();
        this->precomputationList.clear();

        for (const auto& featureName : readFeatures)
        {
            const std::regex numberedf_regex("([A-Za-z]+)\\_([0-9]+)");
            const std::regex namedf_regex("([A-Za-z]+)\\_([a-zA-Z]+)");
            const std::regex simplef_regex("[A-Za-z]+");

            feature_extractor_t currentExtractor = feature_extractor_t::NONE;;

            std::smatch base_match;
            if (std::regex_match(featureName, base_match, numberedf_regex)) {    // Numbered feature
                if (base_match.size() == 3) {
                    size_t currentIndex = std::stoi(base_match[2].str());
                    currentIndex -= 1;  // TODO: comment about 1 based arrays
                    // std::cout << "Numbered Feature, extractor:'"<< base_match[1].str() <<"' index:" << currentIndex << "\n";
                    currentExtractor = getExtractorFromPrefix(base_match[1].str());
                    this->featureList.push_back(new VectorFeature(featureName,currentExtractor,currentIndex));
                }
            } else if (std::regex_match(featureName, base_match, namedf_regex)) {    // Named feature
                if (base_match.size() == 3) {
                    std::string extractorName = base_match[2].str();
                    // std::cout << "Named Feature, extractor:'"<< base_match[1].str() <<"' name:'" << name << "'\n";
                    currentExtractor = getExtractorFromPrefix(base_match[1].str());
                    this->featureList.push_back(new NamedFeature(featureName,currentExtractor,extractorName));
                }
            } else if (std::regex_match(featureName, base_match, simplef_regex)) {    // Single value Feature
                // std::cout << "Simple Feature, extractor:'"<< featureName << "'\n";
                currentExtractor = getExtractorFromPrefix(featureName);
                this->featureList.push_back(new Feature(featureName,currentExtractor));
            } else
                throw std::logic_error("Feature "+featureName+"could not be parsed");
            
            this->precomputationList.insert(currentExtractor);
        }
    }

    ~FeatureParser()
    {
        for (auto pointer : this->featureList)
            delete pointer;
    }
};


} //namespace JsonConf
