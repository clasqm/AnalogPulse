#include <Application.h>
#include <Archivable.h>
#include <AppFileInfo.h>
#include <Message.h>
#include <OS.h>
#include <Screen.h>

#include <stdio.h>

#include "GaugeView.h"
#include "GaugeWindow.h"
#include "AnalogWindow.h"

#define kSignature	"application/x-vnd.takamatsu-analog-ram"

// One pane showing the share of physical memory that is in use.
class _EXPORT RamView : public GaugeView {

	public:
						RamView(BRect);
						RamView(BMessage *data);

static	RamView*		Instantiate(BMessage* data);

	protected:
	virtual	short		SampleValue();
	virtual	const char*	ClassName() const;
	virtual	const char*	AddOnSignature() const;
	virtual	const char*	AppName() const;
	virtual	void		Title(char *buf, size_t size);
	virtual	void		Identification(char *name, size_t nameSize,
							char *extra, size_t extraSize);
};

RamView::RamView(BRect r) : GaugeView(r, 0)
{
	InitObject(r, 0, true);
}

RamView::RamView(BMessage *data) : GaugeView(data)
{
	InitObject(data->FindRect("bounds"), data->FindInt32("cpu"), false);
}

RamView*
RamView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "RamView"))
		return NULL;

	return new RamView(data);
}

short
RamView::SampleValue()
{
	system_info	info;

	if (get_system_info(&info) != B_OK || info.max_pages == 0)
		return 0;

	return (short)((info.used_pages * 100) / info.max_pages);
}

const char*
RamView::ClassName() const
{
	return "RamView";
}

const char*
RamView::AddOnSignature() const
{
	return kSignature;
}

const char*
RamView::AppName() const
{
	return "AnalogRAM";
}

void
RamView::Title(char *buf, size_t size)
{
	snprintf(buf, size, "RAM");
}

void
RamView::Identification(char *name, size_t nameSize, char *extra,
	size_t extraSize)
{
	system_info	info;

	extra[0] = '\0';

	if (get_system_info(&info) != B_OK || info.max_pages == 0) {
		snprintf(name, nameSize, "memory");
		return;
	}

	uint64 bytes = info.max_pages * B_PAGE_SIZE;
	snprintf(name, nameSize, "%llu MB",
		(unsigned long long)(bytes / (1024ULL * 1024ULL)));
}


class RamApplication : public BApplication {

	public:
						RamApplication();
};

RamApplication::RamApplication() : BApplication(kSignature)
{
	BRect	screen = BScreen().Frame();
	BRect	bound(0, 0, kMeterWidth, kMeterHeight);

	bound.OffsetTo(screen.right - kBorderWidth - bound.Width(),
		screen.Height() - bound.Height() - kBorderWidth);

	new GaugeWindow(bound, "AnalogRAM",
		new RamView(BRect(0, 0, kMeterWidth, kMeterHeight)));
}

int
main()
{
	RamApplication	*app;

	app = new RamApplication();
	app->Run();
	delete(app);

	return 0;
}
