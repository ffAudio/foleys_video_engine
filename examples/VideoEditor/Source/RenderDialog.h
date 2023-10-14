/*
  ==============================================================================

    Copyright (c) 2019, Foleys Finest Audio - Daniel Walz
    All rights reserved.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
    OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
    OF THE POSSIBILITY OF SUCH DAMAGE.

  ==============================================================================

    RenderDialog.h
    Created: 23 Apr 2019 8:36:47pm
    Author:  Daniel Walz

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class RenderDialog    : public Component,
                        private Timer
{
public:
    RenderDialog (foleys::ClipRenderer& renderer);

    void resized() override;

    void timerCallback() override;

private:

    void updateGUI();

    foleys::ClipRenderer& renderer;

    Label       filename;
    TextButton  browse { NEEDS_TRANS ("Browse") };
    double      progress = 0.0;
    ProgressBar progressBar { progress };
    TextButton  cancel { NEEDS_TRANS ("Cancel") };
    TextButton  start  { NEEDS_TRANS ("Start") };


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RenderDialog)
};
