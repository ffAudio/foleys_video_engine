#include <juce_gui_extra/juce_gui_extra.h>

using juce::String;

struct FoleysVideoTest final : public juce::JUCEApplication 
{
	const String getApplicationName() final { return "Foleys video engine test"; }

	const String getApplicationVersion() final { return "0.0.1"; }

	void initialise (const String&) final { }

	void shutdown() const final { }
};

START_JUCE_APPLICATION (FoleysVideoTest)
