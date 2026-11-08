#include "GaugeView.h"

#include <Picture.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// The name and its extra value share the top line of the panel, so cut the
// name down until both fit.
static void
clipToWidth(BView* view, char* text, float maxWidth)
{
	size_t length = strlen(text);

	while(length > 0 && view->StringWidth(text) > maxWidth)
	{
		length--;
		// never split a UTF-8 sequence
		while(length > 0 && ((unsigned char)text[length] & 0xC0) == 0x80)
			length--;
		text[length] = '\0';
	}
}

BRect GaugeView::ComputeTick(float vu, BPoint center, long length, float start, float stop)
{
	BRect	r;
	float angle;
	float cosAngle;
	float sinAngle;
	
	angle = (-ScaleAngle(vu) + 90) * kPI / 180.0;
	cosAngle = cos(angle);
	sinAngle = sin(angle);
	r.top = center.y - cosAngle * length * stop;
	r.bottom = center.y - cosAngle * length * start;
	r.right = center.x + sinAngle * length *  start;
	r.left = center.x + sinAngle * length * stop;
	
	return r;
}

BBitmap* GaugeView::MakePanel(BRect r)
{
	BRect			rt;
	short			i;
	float			penSize;
	float			width;
	char			buf[64];
	char			extra[64];
	BFont			font;

	panel = new BBitmap(r, B_COLOR_8_BIT, true);
	view = new BView(r, "", B_FOLLOW_NONE, B_WILL_DRAW);
	panel->AddChild(view);
	panel->Lock();

	BPicture *picture = new BPicture();

	view->BeginPicture(picture);

	//
	// Meter Panel
	//
	const rgb_color kBackColor = {255,255,203,0};
	const rgb_color kForeColor = {0,0,0,0};
	const rgb_color kRedColor = {255,0,0,0};
	
	//const rgb_color kBackColor = {0,0,0,0};
	//const rgb_color kForeColor = {0,255,0,0};
	//const rgb_color kRedColor = {255,0,0,0};
	
	view->SetLowColor(kBackColor);
	view->SetHighColor(kBackColor);
	view->FillRect(r);
	view->SetHighColor(0, 0, 0);
	view->StrokeRect(r);


	penSize = param.panelLength * (kTickEnd - kTickStart);
	rt.Set(param.meterCenter.x - param.panelLength, param.meterCenter.y - param.panelLength,
			param.meterCenter.x + param.panelLength, param.meterCenter.y + param.panelLength);
	rt.InsetBy(penSize / 2, penSize / 2);
	view->SetHighColor(kRedColor);
	view->SetPenSize(penSize);
	view->StrokeArc(rt, ScaleAngle(81), ScaleAngle(100) - ScaleAngle(80));
	
	view->SetPenSize(1);
	view->SetHighColor(kForeColor);
	for(i = 0; i <= 100; i = i + 10)
	{
		rt = ComputeTick(i, param.meterCenter, param.panelLength, kTickStart, kTickEnd);
		view->StrokeLine(rt.RightBottom(), rt.LeftTop());	
	}
	for(i = 0; i <= 100; i = i + 5)
	{
		rt = ComputeTick(i, param.meterCenter, param.panelLength, kTickStart, kTickMiddle);
		view->StrokeLine(rt.RightBottom(), rt.LeftTop());	
	}

	font.SetFamilyAndStyle("Noto Serif", "Italic");
	view->SetFont(&font, B_FONT_ALL);
	view->SetFontSize(9);
	
	for(i = 0; i <= 100; i = i + 10)
	{
		rt = ComputeTick(i, param.meterCenter, param.panelLength+5, kTickStart, kTickEnd);
		sprintf(buf, "%d", i);
		width = view->StringWidth(buf) / 2;
		switch(i)
		{
			case 0:
				view->MovePenTo(rt.left - width - 1, rt.top);
				break;
			case 10:
				view->MovePenTo(rt.left - width - 2, rt.top - 1);
				break;
			case 90:
				view->MovePenTo(rt.left - width + 2, rt.top - 1);
				break;
			case 100:
				view->MovePenTo(rt.left - width + 5, rt.top);
				break;
			default:
				view->MovePenTo(rt.left - width, rt.top);
		}
		view->DrawString(buf);
	}

	rt = r;


	font.SetFamilyAndStyle("Noto Serif", "Bold");
	view->SetFont(&font, B_FONT_ALL);
	view->SetFontSize(12);
	Title(buf, sizeof(buf));
	view->MovePenTo(param.meterCenter.x - view->StringWidth(buf)/ 2, 75 + kGaugeGap);
	view->DrawString(buf);
	
	// Every pane identifies what it measures, centred along the top.
	font.SetFamilyAndStyle("Noto Sans", "Regular");
	view->SetFont(&font);
	view->SetFontSize(9);

	Identification(buf, sizeof(buf), extra, sizeof(extra));

	if(extra[0] != '\0')
	{
		clipToWidth(view, buf, rt.Width() - view->StringWidth(extra) - 8);

		float nameClockGap = 4;
		float lineWidth = view->StringWidth(buf) + nameClockGap
			+ view->StringWidth(extra);
		float x = rt.left + (rt.Width() - lineWidth) / 2;
		view->MovePenTo(x, 12);
		view->DrawString(buf);
		view->MovePenTo(x + view->StringWidth(buf) + nameClockGap, 12);
		view->DrawString(extra);
	}
	else
	{
		// Nothing to put beside the name: centre it on its own.
		clipToWidth(view, buf, rt.Width() - 6);
		view->MovePenTo(rt.left + (rt.Width() - view->StringWidth(buf)) / 2, 12);
		view->DrawString(buf);
	}
	
	view->EndPicture();
	view->DrawPicture(picture, BPoint(0,0));
	view->Sync();
	panel->Unlock();

	delete picture;

	return panel;
}
