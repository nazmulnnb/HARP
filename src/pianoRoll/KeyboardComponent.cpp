#include "KeyboardComponent.hpp"


KeyboardComponent::KeyboardComponent ()
{
    blackPitches.add(1);
    blackPitches.add(3);
    blackPitches.add(6);
    blackPitches.add(8);
    blackPitches.add(10);
}

void KeyboardComponent::paint(Graphics& g)
{
    const float noteCompHeight = getHeight() / 128.0;

    float line = 0; // Cumulative height
    
    for (int i = 127; i >= 0; i--) {
        const int pitch = i % 12;

        g.setColour(blackPitches.contains(pitch) ? Colours::darkgrey : Colours::lightgrey.darker());
        g.fillRect(0, (int) line, getWidth(), (int) noteCompHeight);

        g.setColour(Colours::white);
        // TODO - add note name
        g.drawText(String(i), 5, line, 40, noteCompHeight, Justification::left);

        line += noteCompHeight;

        g.setColour(Colours::black);
        g.drawLine(0, line, getWidth(), line);
    }
}
