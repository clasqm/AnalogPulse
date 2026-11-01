#ifndef WINDOW_H
#define WINDOW_H
#endif

#include <Window.h>

#define kMeterWidth		200.
// 90 pixels plus the kGaugeGap of breathing room under the CPU identification
#define kMeterHeight	95.
#define kBorderWidth	5.

class AnalogWindow : public BWindow {

	public:
						AnalogWindow(BRect, const char *);
		bool			QuitRequested();
};

