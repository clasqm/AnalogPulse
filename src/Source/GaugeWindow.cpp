#include "GaugeWindow.h"

#include <Application.h>

GaugeWindow::GaugeWindow(BRect bound, const char *title, BView *pane)
	 :BWindow(bound, title, B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	Lock();

	AddChild(pane);

	// Poll ten times a second: memory and volume use move slowly, but a
	// coarser interval makes short spikes easy to miss.
	SetPulseRate(100000.0);

	Unlock();

	Show();
}

bool
GaugeWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return TRUE;
}
