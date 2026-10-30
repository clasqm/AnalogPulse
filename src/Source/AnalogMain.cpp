#include "AnalogPulse.h"
#include <Screen.h>
#include <Rect.h>

PApplication::PApplication() : BApplication("application/x-vnd.ebata-analogpulse")
{
	BRect			bound;
	BRect 			info = (new BScreen())->Frame();
	system_info		sysInfo;

	//get_screen_info(&info);
	get_system_info(&sysInfo);
	bound.Set(0, 0, kMeterWidth * sysInfo.cpu_count, kMeterHeight);
	bound.OffsetTo(info.right - kBorderWidth - bound.Width(),
						info.Height() - bound.Height() - kBorderWidth);

	// The window shows itself and stays alive for the life of the app.
	new AnalogWindow(bound, "AnalogPulse Lite");
}

int main()
{
	PApplication	*app;
	
	app = new PApplication();
	app->Run();
	delete(app);
		
	return 0;
}
