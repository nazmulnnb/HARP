#include "PlaybackRegion.h"

PlaybackRegionView::PlaybackRegionView(EditorView &editorView,
                                       ARAPlaybackRegion &region,
                                       WaveformCache &cache)
    : araEditorView(editorView), playbackRegion(region), waveformCache(cache),
      previewRegionOverlay(*this) {

  ARAAudioSource *audioSource;

  auto audioModification = playbackRegion.getAudioModification();
  auto audioModificationInstance =
      dynamic_cast<AudioModification *>(audioModification);

  audioSource = playbackRegion.getAudioModification()->getAudioSource();

  waveformCache.getOrCreateThumbnail(audioSource, audioModificationInstance)
      .addChangeListener(this);

  audioSource->addListener(this);
  playbackRegion.addListener(this);
  araEditorView.addListener(this);
  addAndMakeVisible(previewRegionOverlay);

  setTooltip("Double-click to toggle dim state of the region, click and hold "
             "to prelisten region near click.");
}

PlaybackRegionView::~PlaybackRegionView() {
  auto *audioSource = playbackRegion.getAudioModification()->getAudioSource();

  audioSource->removeListener(this);
  playbackRegion.removeListener(this);
  araEditorView.removeListener(this);

  waveformCache
      .getOrCreateThumbnail(audioSource,
                            dynamic_cast<AudioModification *>(
                                playbackRegion.getAudioModification()))
      .removeChangeListener(this);
}

void PlaybackRegionView::mouseDown(const juce::MouseEvent &m) {
  const auto relativeTime =
      (double)m.getMouseDownX() / getLocalBounds().getWidth();
  const auto previewTime =
      playbackRegion.getStartInPlaybackTime() +
      relativeTime * playbackRegion.getDurationInPlaybackTime();
  auto &previewState =
      ARADocumentControllerSpecialisation::getSpecialisedDocumentController<
          TensorJuceDocumentControllerSpecialisation>(
          playbackRegion.getDocumentController())
          ->previewState;
  previewState.previewTime.store(previewTime);
  previewState.previewedRegion.store(&playbackRegion);
  previewRegionOverlay.update();
}

void PlaybackRegionView::mouseUp(const juce::MouseEvent &) {
  auto &previewState =
      ARADocumentControllerSpecialisation::getSpecialisedDocumentController<
          TensorJuceDocumentControllerSpecialisation>(
          playbackRegion.getDocumentController())
          ->previewState;
  previewState.previewTime.store(0.0);
  previewState.previewedRegion.store(nullptr);
  previewRegionOverlay.update();
}

void PlaybackRegionView::mouseDoubleClick(const juce::MouseEvent &) {
  // Set the dim flag on our region's audio modification when double-clicked
  auto audioModification =
      playbackRegion.getAudioModification<AudioModification>();
  audioModification->setDimmed(!audioModification->isDimmed());

  // Send a content change notification for the modification and all associated
  // playback regions
  audioModification->notifyContentChanged(
      ARAContentUpdateScopes::samplesAreAffected(), true);
  for (auto region : audioModification->getPlaybackRegions())
    region->notifyContentChanged(ARAContentUpdateScopes::samplesAreAffected(),
                                 true);
}

void PlaybackRegionView::changeListenerCallback(juce::ChangeBroadcaster *) {
  repaint();
}

void PlaybackRegionView::didEnableAudioSourceSamplesAccess(ARAAudioSource *,
                                                           bool) {
  repaint();
}

void PlaybackRegionView::willUpdatePlaybackRegionProperties(
    ARAPlaybackRegion *, ARAPlaybackRegion::PropertiesPtr newProperties) {
  if (playbackRegion.getName() != newProperties->name ||
      playbackRegion.getColor() != newProperties->color) {
    repaint();
  }
}

void PlaybackRegionView::didUpdatePlaybackRegionContent(
    ARAPlaybackRegion *, ARAContentUpdateScopes) {
  repaint();
}

void PlaybackRegionView::onNewSelection(const ARAViewSelection &viewSelection) {
  const auto &selectedPlaybackRegions = viewSelection.getPlaybackRegions();
  const bool selected =
      std::find(selectedPlaybackRegions.begin(), selectedPlaybackRegions.end(),
                &playbackRegion) != selectedPlaybackRegions.end();
  if (selected != isSelected) {
    isSelected = selected;
    repaint();
  }
}

void PlaybackRegionView::paint(juce::Graphics &g) {
  g.fillAll(convertOptionalARAColour(playbackRegion.getEffectiveColor(),
                                     Colours::black));

  const auto *audioModification =
      playbackRegion.getAudioModification<AudioModification>();
  g.setColour(audioModification->isDimmed() ? Colours::whitesmoke.darker()
                                            : Colours::whitesmoke.brighter());

  if (audioModification->getAudioSource()->isSampleAccessEnabled()) {
    auto &thumbnail = waveformCache.getOrCreateThumbnail(
        playbackRegion.getAudioModification()->getAudioSource(),
        dynamic_cast<AudioModification *>(
            playbackRegion.getAudioModification()));
    thumbnail.drawChannels(
        g, getLocalBounds(), playbackRegion.getStartInAudioModificationTime(),
        playbackRegion.getEndInAudioModificationTime(), 1.0f);
  } else {
    g.setFont(Font(12.0f));
    g.drawText("Audio Access Disabled", getLocalBounds(),
               Justification::centred);
  }

  g.setColour(Colours::white.withMultipliedAlpha(0.9f));
  g.setFont(Font(12.0f));
  g.drawText(convertOptionalARAString(playbackRegion.getEffectiveName()),
             getLocalBounds(), Justification::topLeft);

  if (audioModification->isDimmed())
    g.drawText("using libtorch", getLocalBounds(), Justification::bottomLeft);

  g.setColour(isSelected ? Colours::white : Colours::black);
  g.drawRect(getLocalBounds());
}

void PlaybackRegionView::resized() { repaint(); }

TensorJuceDocumentControllerSpecialisation *
PlaybackRegionView::getDocumentController() const {
  return ARADocumentControllerSpecialisation::getSpecialisedDocumentController<
      TensorJuceDocumentControllerSpecialisation>(
      playbackRegion.getDocumentController());
}
