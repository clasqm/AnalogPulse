#include "GaugeView.h"

#include <Autolock.h>
#include <Alert.h>
#include <Dragger.h>
#include <Message.h>
#include <Window.h>

#include <math.h>

GaugeView::GaugeView(BRect r, int id)
		: _inherited(r, "", B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED)
{
	// Nothing is initialised here: InitObject() draws the panel and calls
	// virtual methods, so the subclass has to invoke it from its own
	// constructor body once the object is fully built.
	panel = NULL;
	view = NULL;
	oldCPUTime = 0;
	param.cpu = id;
}

GaugeView::GaugeView(BMessage *data) : _inherited(data)
{
	panel = NULL;
	view = NULL;
	oldCPUTime = 0;
}

GaugeView::~GaugeView()
{
	delete panel;
}

void
GaugeView::InitObject(BRect rt, int id, bool withDragger)
{
	rt.OffsetTo(B_ORIGIN);
	param.cpu = id;

	oldCPUTime = 0;

	param.panelLength = rt.Width() / 2 - 3 * 7;
	param.needle = (rt.Width() / 2 - 3 * 7) * 0.93;
	param.meterCenter.Set(rt.Width() / 2,
		param.panelLength + 3 * 8 + kGaugeGap);
	MakeTable();

	MakePanel(rt);

	SetViewColor(216, 216, 216);
	SetLowColor(216, 216, 216);
	SetHighColor(0, 0, 0);

	if (withDragger) {
		BRect	drag = Bounds();
		drag.left = drag.right - 7;
		drag.top = drag.bottom - 7;
		drag.OffsetBy(-1, -1);
		AddChild(new BDragger(drag, this));
	}
}

status_t
GaugeView::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);

	data->AddString("class", ClassName());
	data->AddString("add_on", AddOnSignature());
	data->AddRect("bounds", Bounds());
	data->AddInt32("cpu", param.cpu);
	return B_NO_ERROR;
}

void
GaugeView::Pulse()
{
	UpdateNeedle(SampleValue());
}

void
GaugeView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_ABOUT_REQUESTED:
			(new BAlert("", AppName(), "OK"))->Go();
			break;

		default:
			_inherited::MessageReceived(message);
	}
}

void
GaugeView::UpdateNeedle(short cpuTime)
{
	BRect	updateRect;
	BRect	rt;

	if (cpuTime < 0)
		cpuTime = 0;
	if (cpuTime > 100)
		cpuTime = 100;

	if (cpuTime != oldCPUTime) {
		rt.SetRightBottom(param.meterCenter);
		rt.SetLeftTop(angle[oldCPUTime].arry);
		if (rt.left > rt.right) {
			double tmp = rt.left;
			rt.left = rt.right;
			rt.right = tmp;
		}

		DrawBitmap(panel, rt, rt);

		const rgb_color kNeedleColor = {0, 0, 0, 0};
		this->SetHighColor(kNeedleColor);

		StrokeLine(param.meterCenter, angle[cpuTime].arry);
	}
	Sync();
	oldCPUTime = cpuTime;
}

void
GaugeView::Draw(BRect updateRect)
{
	BAutolock lock(Window());
	if (lock.IsLocked())
		DrawBitmapAsync(panel, BPoint(0, 0));
}

float
GaugeView::ScaleAngle(float dB)
{
	return(180 - 15 - kSCALE_DEGREES / 100 * dB);
}

void
GaugeView::MakeTable(void)
{
	float	ang;

	for (short i = 0; i <= 100; i++) {
		ang = (-ScaleAngle(i) + 90) * kPI / 180.0;
		angle[i].arry.Set(param.meterCenter.x + sin(ang) * param.needle,
							param.meterCenter.y - cos(ang) * param.needle);
	}
}
