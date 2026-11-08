#include <Application.h>
#include <Archivable.h>
#include <AppFileInfo.h>
#include <FindDirectory.h>
#include <Message.h>
#include <OS.h>
#include <Path.h>
#include <Screen.h>

#include <stdio.h>
#include <sys/statvfs.h>

#include "GaugeView.h"
#include "GaugeWindow.h"
#include "AnalogWindow.h"

#define kSignature	"application/x-vnd.takamatsu-analog-disk"

// One pane showing how full the volume Haiku boots from is.
static bool
bootDiskUsage(short* percent, uint64* totalBytes)
{
	BPath	path;

	if (find_directory(B_SYSTEM_DIRECTORY, &path) != B_OK)
		return false;

	struct statvfs	vfs;
	if (statvfs(path.Path(), &vfs) != 0)
		return false;

	uint64 total = vfs.f_blocks;
	if (total == 0)
		return false;

	uint64 freeBlocks = vfs.f_bfree;
	uint64 unit = vfs.f_frsize != 0 ? vfs.f_frsize : vfs.f_bsize;

	*percent = (short)(((total - freeBlocks) * 100) / total);
	*totalBytes = total * unit;
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
	uint64	total;

	if (!bootDiskUsage(&percent, &total))
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
	uint64	total = 0;

	extra[0] = '\0';

	if (!bootDiskUsage(&percent, &total)) {
		snprintf(name, nameSize, "boot disk");
		return;
	}

	if (total >= 1000ULL * 1024 * 1024 * 1024)
		snprintf(name, nameSize, "%llu TB",
			(unsigned long long)(total / (1024ULL * 1024ULL * 1024ULL * 1024ULL)));
	else
		snprintf(name, nameSize, "%llu GB",
			(unsigned long long)(total / (1024ULL * 1024ULL * 1024ULL)));
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
