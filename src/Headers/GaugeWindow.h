#ifndef GAUGEWINDOW_H
#define GAUGEWINDOW_H

#include <View.h>
#include <Window.h>

// A window holding one gauge pane.  AnalogPulse has its own window class
// because it puts one pane per CPU side by side.
class GaugeWindow : public BWindow {

	public:
						GaugeWindow(BRect bound, const char *title,
							BView *pane);

	virtual	bool		QuitRequested();
};

#endif // GAUGEWINDOW_H
