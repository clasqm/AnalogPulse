#include <Application.h>
#include <Archivable.h>
#include <AppFileInfo.h>
#include <Message.h>
#include <OS.h>
#include <Screen.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <stdio.h>

#include "GaugeView.h"
#include "GaugeWindow.h"
#include "AnalogWindow.h"

#define kSignature	"application/x-vnd.takamatsu-analog-disk"

// One pane showing how full the volume Haiku boots from is.  The boot volume
// is asked for directly: going through a path (statvfs on the system
// directory) also drags in path resolution, which can fail and then leaves
// the gauge with nothing to show.
static bool
bootVolumeUsage(short* percent, uint64* capacity)
{
	BVolumeRoster	roster;
	BVolume			volume;

	if (roster.GetBootVolume(&volume) != B_OK)
		return false;

	off_t total = volume.Capacity();
	off_t freeBytes = volume.FreeBytes();
	if (total <= 0 || freeBytes < 0 || freeBytes > total)
		return false;

	*percent = (short)(((total - freeBytes) * 100) / total);
	*capacity = (uint64)total;
	return true;
}

class _EXPORT DiskView : public GaugeView {

	public:
						DiskView(BRect);
						DiskView(BMessage *data);

static	DiskView*		Instantiate(BMessage* data);

	protected:
	virtual	short		SampleValue();
	virtual	const char*	ClassName() const;
	virtual	const char*	AddOnSignature() const;
	virtual	const char*	AppName() const;
	virtual	void		Title(char *buf, size_t size);
	virtual	void		Identification(char *name, size_t nameSize,
							char *extra, size_t extraSize);
};

DiskView::DiskView(BRect r) : GaugeView(r, 0)
{
	InitObject(r, 0, true);
}

DiskView::DiskView(BMessage *data) : GaugeView(data)
{
	InitObject(data->FindRect("bounds"), data->FindInt32("cpu"), false);
}

DiskView*
DiskView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "DiskView"))
		return NULL;

	return new DiskView(data);
}

short
DiskView::SampleValue()
{
	short	percent;
	uint64	capacity;

	if (!bootVolumeUsage(&percent, &capacity))
		return 0;

	return percent;
}

const char*
DiskView::ClassName() const
{
	return "DiskView";
}

const char*
DiskView::AddOnSignature() const
{
	return kSignature;
}

const char*
DiskView::AppName() const
{
	return "AnalogDisk";
}

void
DiskView::Title(char *buf, size_t size)
{
	snprintf(buf, size, "DISK");
}

void
DiskView::Identification(char *name, size_t nameSize, char *extra,
	size_t extraSize)
{
	short	percent;
	uint64	capacity = 0;

	extra[0] = '\0';
	snprintf(name, nameSize, "/boot");

	if (!bootVolumeUsage(&percent, &capacity))
		return;

	if (capacity >= 1024ULL * 1024 * 1024 * 1024)
		snprintf(extra, extraSize, "%.1f TB",
			(double)(capacity / (1024ULL * 1024 * 1024)) / 1024.0);
	else
		snprintf(extra, extraSize, "%llu GB",
			(unsigned long long)(capacity / (1024ULL * 1024 * 1024)));
}


class DiskApplication : public BApplication {

	public:
						DiskApplication();
};

DiskApplication::DiskApplication() : BApplication(kSignature)
{
	BRect	screen = BScreen().Frame();
	BRect	bound(0, 0, kMeterWidth, kMeterHeight);

	bound.OffsetTo(screen.right - kBorderWidth - bound.Width(),
		screen.Height() - bound.Height() - kBorderWidth);

	new GaugeWindow(bound, "AnalogDisk",
		new DiskView(BRect(0, 0, kMeterWidth, kMeterHeight)));
}

int
main()
{
	DiskApplication	*app;

	app = new DiskApplication();
	app->Run();
	delete(app);

	return 0;
}
