#ifndef pVIEW_H
#include "AnalogView.h"
#endif
#ifndef pWINDOW_H
#include "AnalogWindow.h"
#endif

#include <Application.h>

AnalogWindow::AnalogWindow(BRect bound, const char *title)
	 :BWindow(bound, title, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	AnalogView	*analog;
	system_info	sysInfo;
	BRect		rect;

	Lock();

	bound.OffsetTo(B_ORIGIN);
	get_system_info(&sysInfo);	
	rect.Set(0, 0, kMeterWidth, kMeterHeight);

	for(uint32 cpu = 0; cpu < sysInfo.cpu_count; cpu++)
	{
		analog = new AnalogView(rect, (int)cpu);
		AddChild(analog);
		rect.OffsetBy(kMeterWidth, 0);
	}

	SetPulseRate(200000.0);

	Unlock();

	Show();
}

bool AnalogWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return TRUE;
}