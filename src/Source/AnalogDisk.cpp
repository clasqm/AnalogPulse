#include <Alert.h>
#include <Application.h>
#include <Archivable.h>
#include <AppFileInfo.h>
#include <Message.h>
#include <OS.h>
#include <Screen.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <stdio.h>
#include <string.h>

#include "GaugeView.h"
#include "GaugeWindow.h"
#include "AnalogWindow.h"

#define kSignature	"application/x-vnd.takamatsu-analog-disk"

// One pane showing how full the volume Haiku boots from is.  The boot volume
// is asked for directly: going through a path (statvfs on the system
// directory) also drags in path resolution, which can fail and then leaves
// the gauge with nothing to show.
//
// A failure says why in problem, so SampleValue() can report it once.  Without
// that a gauge stuck at 0 is indistinguishable from a broken volume query.
static bool
queryBootVolume(short* percent, uint64* capacity, char* problem,
	size_t problemSize)
{
	BVolumeRoster	roster;
	BVolume			volume;
	status_t		status;

	problem[0] = '\0';

	status = roster.GetBootVolume(&volume);
	if (status != B_OK) {
		snprintf(problem, problemSize,
			"BVolumeRoster::GetBootVolume() failed: %s (0x%08lx)",
			strerror(status), (unsigned long)status);
		return false;
	}

	off_t total = volume.Capacity();
	off_t freeBytes = volume.FreeBytes();

	if (total <= 0) {
		snprintf(problem, problemSize,
			"BVolume::Capacity() returned %lld (%s, 0x%08lx)",
			(long long)total, strerror((status_t)total),
			(unsigned long)total);
		return false;
	}

	if (freeBytes < 0) {
		snprintf(problem, problemSize,
			"BVolume::FreeBytes() returned %lld (%s, 0x%08lx)",
			(long long)freeBytes, strerror((status_t)freeBytes),
			(unsigned long)freeBytes);
		return false;
	}

	if (freeBytes > total) {
		snprintf(problem, problemSize,
			"BVolume::FreeBytes() (%lld) is larger than Capacity() (%lld)",
			(long long)freeBytes, (long long)total);
		return false;
	}

	*percent = (short)(((total - freeBytes) * 100) / total);
	*capacity = (uint64)total;
	return true;
}

static bool
bootVolumeUsage(short* percent, uint64* capacity)
{
	char	problem[256];

	return queryBootVolume(percent, capacity, problem, sizeof(problem));
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

// Temporary diagnostics: an unlit gauge looks the same whether the volume
// query failed or the volume is genuinely empty, so the first failure is
// reported out loud.  Go() blocks the pane until the button is pressed and
// deletes the alert itself; the diagnosis also goes to stderr.  Delete this
// together with the problem reporting in queryBootVolume() once the volume
// reading is known to work everywhere.
static bool sProblemReported = false;

short
DiskView::SampleValue()
{
	short	percent;
	uint64	capacity;
	char	problem[256];

	if (queryBootVolume(&percent, &capacity, problem, sizeof(problem)))
		return percent;

	if (!sProblemReported) {
		sProblemReported = true;
		fprintf(stderr, "AnalogDisk: %s\n", problem);
		BAlert *alert = new BAlert("AnalogDisk", problem, "OK", NULL, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		if (alert != NULL)
			alert->Go();
	}

	return 0;
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
