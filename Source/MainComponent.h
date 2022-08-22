#pragma once

#include <JuceHeader.h>
#include <thread>


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/

class MainComponent  : public juce::AudioAppComponent
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // Your private member variables go here...


    juce::TextButton openButton;
    void openButtonClicked();

    juce::TextButton glitchButton;

    void glitchGenerator(int numThreads);
    void glitchButtonClicked();

    std::unique_ptr<juce::FileChooser> chooser;
    juce::AudioFormatManager formatManager;

    std::vector<std::thread*> threadVector;
    std::vector<int> delayVector;
    juce::Slider numThreadsSlider;
    juce::Label numThreadsLabel;
    juce::Slider delaySlider;
    juce::Label delayLabel;
    juce::Slider randomnessSlider;
    juce::Label randomnessLabel;

    std::vector<float> outputVector;
    std::mutex vectorMutex;
    int delaySamples = 22050;
    bool playbackTrigger = false;
    int position = 0;
    juce::AudioBuffer<float> inputBuffer;
    juce::AudioBuffer<float> outputBuffer;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
