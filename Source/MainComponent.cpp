#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    addAndMakeVisible(&openButton);
    openButton.setButtonText("Open...");
    openButton.onClick = [this] { openButtonClicked(); };

    addAndMakeVisible(&glitchButton);
    glitchButton.setButtonText("Glitch...");
    glitchButton.onClick = [this] { glitchButtonClicked(); };

    addAndMakeVisible(&numThreadsSlider);
    numThreadsSlider.setRange(1, 10, 1);
    addAndMakeVisible(numThreadsLabel);
    numThreadsLabel.setText("Number of Threads", juce::dontSendNotification);
    numThreadsLabel.attachToComponent(&numThreadsSlider, true);
    
    addAndMakeVisible(&delaySlider);
    delaySlider.setRange(0.01, 1.0);
    addAndMakeVisible(delayLabel);
    delayLabel.setText("Delay", juce::dontSendNotification);
    delayLabel.attachToComponent(&delaySlider, true);
    
    addAndMakeVisible(&randomnessSlider);
    randomnessSlider.setRange(0.0, 1.0);
    addAndMakeVisible(randomnessLabel);
    randomnessLabel.setText("Randomness", juce::dontSendNotification);
    randomnessLabel.attachToComponent(&randomnessSlider, true);

    addAndMakeVisible(&spreadSlider);
    spreadSlider.setRange(0.0, 1.0);
    addAndMakeVisible(spreadLabel);
    spreadLabel.setText("Spread", juce::dontSendNotification);
    spreadLabel.attachToComponent(&spreadSlider, true);

    formatManager.registerBasicFormats();

    setSize (800, 600);

    setAudioChannels (0, 2);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    outputBuffer.setSize(2, 0);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{

    auto numInputChannels = outputBuffer.getNumChannels();
    auto numOutputChannels = bufferToFill.buffer->getNumChannels();

    auto outputSamplesRemaining = bufferToFill.numSamples;
    auto outputSamplesOffset = bufferToFill.startSample;

    if (playbackTrigger) {
        while (outputSamplesRemaining > 0)
        {
            auto bufferSamplesRemaining = outputBuffer.getNumSamples() - position;
            auto samplesThisTime = juce::jmin(outputSamplesRemaining, bufferSamplesRemaining);

            bufferToFill.buffer->copyFrom(0, outputSamplesOffset, outputBuffer, 0, position, samplesThisTime);
            bufferToFill.buffer->copyFrom(1, outputSamplesOffset, outputBuffer, 1, position, samplesThisTime);

            outputSamplesRemaining -= samplesThisTime;
            outputSamplesOffset += samplesThisTime;
            position += samplesThisTime;

            if (position == outputBuffer.getNumSamples()) {
                playbackTrigger = false;
                position = 0;
            }
        }
    }
}

void MainComponent::releaseResources()
{
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    openButton.setBounds(area.removeFromTop(50).reduced(5));
    glitchButton.setBounds(area.removeFromTop(50).reduced(5));

    numThreadsSlider.setBounds(area.removeFromTop(50).removeFromRight(getWidth()-100).reduced(5));
    delaySlider.setBounds(area.removeFromTop(50).removeFromRight(getWidth() - 100).reduced(5));
    randomnessSlider.setBounds(area.removeFromTop(50).removeFromRight(getWidth() - 100).reduced(5));
    spreadSlider.setBounds(area.removeFromTop(50).removeFromRight(getWidth() - 100).reduced(5));
}

////

void MainComponent::openButtonClicked()
{
    chooser = std::make_unique<juce::FileChooser>("Select a Wave file to play...",
        juce::File{},
        "*.wav");                   
    auto chooserFlags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)           
        {
            auto file = fc.getResult();

            if (file != juce::File{})                                                     
            {
                juce::ScopedPointer<juce::AudioFormatReader> reader = formatManager.createReaderFor(file);

                if (reader != nullptr)
                {
                    inputBuffer.setSize(2, reader->lengthInSamples);
                    reader->read(inputBuffer.getArrayOfWritePointers(), 2, 0, reader->lengthInSamples);
                }
            }
        });
}

void MainComponent::glitchGenerator(int threadsLeft)
{
    int bufferSize = inputBuffer.getNumSamples();
    float delay = delaySlider.getValue();
    float randomness = randomnessSlider.getValue() * juce::Random::getSystemRandom().nextFloat();

    juce::Random random{};
    float spreadL = 1;
    float spreadR = 1;
    if (random.nextBool()) 
    {
        spreadL -= spreadSlider.getValue() * random.nextFloat();
    }
    else {
        spreadR -= spreadSlider.getValue() * random.nextFloat();
    }

    int delaySample = 44100 * (1 - randomness) * delay;
    std::thread newThread;

    for (int i = 0; i < bufferSize; ++i)
    {

        vectorMutex.lock();
        outputVectorL.push_back(inputBuffer.getSample(0, i) * spreadL);
        outputVectorR.push_back(inputBuffer.getSample(1, i) * spreadR);
        vectorMutex.unlock();

        if (threadsLeft > 1 && i == delaySample)
        {
            newThread = std::thread([this, threadsLeft] { glitchGenerator(threadsLeft-1); });
        }
    }

    if (newThread.joinable()) {
        newThread.join();
    }
}

void MainComponent::glitchButtonClicked()
{
    outputVectorL.clear();
    outputVectorR.clear();

    int numThreads = numThreadsSlider.getValue();

    std::thread rootThread = std::thread([this, numThreads] {glitchGenerator(numThreads); });

    rootThread.join();

    outputBuffer.setSize(2, outputVectorL.size());
    for (int i = 0; i < outputBuffer.getNumSamples(); ++i) {
        outputBuffer.setSample(0, i, outputVectorL.at(i));
        outputBuffer.setSample(1, i, outputVectorR.at(i));
    }

    playbackTrigger = true;
    position = 0;
}



