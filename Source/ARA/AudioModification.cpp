/**
 * @file
 * @brief This file is part of the JUCE examples.
 *
 * Copyright (c) 2022 - Raw Material Software Limited
 * The code included in this file is provided under the terms of the ISC license
 * http://www.isc.org/downloads/software-support-policy/isc-license. Permission
 * To use, copy, modify, and/or distribute this software for any purpose with or
 * without fee is hereby granted provided that the above copyright notice and
 * this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
 * WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
 * PURPOSE, ARE DISCLAIMED.
 *
 * Audio modification class started from the ARAPluginDemo.h file provided by
 * juce. This class now also manages offline processing.
 * @author JUCE, aldo aguilar, hugo flores garcia
 */

#include "AudioModification.h"

/**
 * @brief Construct a new AudioModification object
 *
 * @param audioSource Pointer to the ARAAudioSource
 * @param hostRef A reference to ARAAudioModificationHost
 * @param optionalModificationToClone A pointer to an optional modification to
 * clone
 */
AudioModification::AudioModification(
    juce::ARAAudioSource *audioSource, ARA::ARAAudioModificationHostRef hostRef,
    const ARAAudioModification *optionalModificationToClone, 
    std::shared_ptr<TorchWave2Wave> model
    )
    : ARAAudioModification(audioSource, hostRef, optionalModificationToClone), 
      mModel(model) {

  DBG("AudioModification::created");
  DBG("AudioModification::the audio source is " << audioSource->getName());
  DBG("AudioModification:: create reader for " << audioSource->getName());
  mAudioSourceReader = std::make_unique<juce::ARAAudioSourceReader>(audioSource);
  mSampleRate = audioSource->getSampleRate();
  mAudioSourceName = audioSource->getName();
}


bool AudioModification::isDimmed() const { return dimmed; }

void AudioModification::setDimmed(bool shouldDim) { dimmed = shouldDim; }

std::string AudioModification::getSourceName() { return mAudioSourceName; }

/**
 * @brief Process audio from audio source with deep learning effect
 *
 * @param params Map of parameters for learner
 */
void AudioModification::process(std::map<std::string, std::any> &params) {
  if (!mModel->ready()) {
    return;
  }

  if (!mAudioSourceReader->isValid())
    DBG("AudioModification:: invalid audio source reader");
  else {
    auto numChannels = mAudioSourceReader->numChannels;
    auto numSamples = mAudioSourceReader->lengthInSamples;
    auto sampleRate = mSampleRate;

    DBG("AudioModification:: audio source: "
        << mAudioSourceName << " channels: " << juce::String(numChannels)
        << " length in samples: " << juce::String(numSamples));

    mAudioBuffer.reset(new juce::AudioBuffer<float>(numChannels, numSamples));

    // reading into audio buffer
    mAudioSourceReader->read(mAudioBuffer.get(), 0,
                             static_cast<int>(numSamples), 0, true, true);
    mModel->process(mAudioBuffer.get(), sampleRate, params);

    // connect the modified buffer to the source

    mIsModified = true;
  }
}

void AudioModification::load(std::map<std::string, std::any> &params) {
  // get the modelPath, pass it to the model
  DBG("AudioModification::load");
  mModel->load(params);
}

juce::AudioBuffer<float> *AudioModification::getModifiedAudioBuffer() {
  return mAudioBuffer.get();
}

bool AudioModification::getIsModified() { return mIsModified; }

// void AudioModification::provideTheEditor(EditorRenderer *editorRenderer
//   mModel->addChangeListener(listener);
// }

void AudioModification::sendTheCallbackToTorchModel(std::function<void(std::string)> callback) { //std::function<void(std::unique_ptr<Module>&)>
    mModel->setTheCallbackFromAudioModification(callback);
  }