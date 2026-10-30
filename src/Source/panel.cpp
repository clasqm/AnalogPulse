#ifndef pVIEW_H
#include "AnalogView.h"
#endif

#include <Picture.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

const char *gProcessorName[] = {
		"unknown",
		"601",
		"603",
		"603e",
		"604",
		"604e",
		"unknown",
		"unknown",
		"unknown",
		"unknown",
		"unknown",
		"unknown",
		"unknown"
};


BRect AnalogView::computeTick(float vu, BPoint center, long length, float start, float stop)
{
	BRect	r;
	float angle;
	float cosAngle;
	float sinAngle;
	
	angle = (-scaleAngle(vu) + 90) * kPI / 180.0;
	cosAngle = cos(angle);
	sinAngle = sin(angle);
	r.top = center.y - cosAngle * length * stop;
	r.bottom = center.y - cosAngle * length * start;
	r.right = center.x + sinAngle * length *  start;
	r.left = center.x + sinAngle * length * stop;
	
	return r;
}

// Haiku dropped system_info::cpu_type, ::cpu_clock_speed and ::cpu_infos.
// CPU identification and clock speed now come from the CPU topology API.
static const char*
cpuPlatformName(enum cpu_platform platform)
{
	switch(platform)
	{
		case B_CPU_x86:		return "x86";
		case B_CPU_x86_64:	return "x86_64";
		case B_CPU_PPC:		return "PowerPC";
		case B_CPU_PPC_64:	return "PowerPC 64";
		case B_CPU_M68K:	return "m68k";
		case B_CPU_ARM:		return "ARM";
		case B_CPU_ARM_64:	return "ARM 64";
		case B_CPU_ALPHA:	return "Alpha";
		case B_CPU_MIPS:	return "MIPS";
		case B_CPU_SH:		return "SH";
		case B_CPU_SPARC:	return "SPARC";
		case B_CPU_RISC_V:	return "RISC-V";
		default:			return "unknown";
	}
}

static const char*
cpuVendorName(enum cpu_vendor vendor)
{
	switch(vendor)
	{
		case B_CPU_VENDOR_AMD:		return "AMD";
		case B_CPU_VENDOR_CYRIX:	return "Cyrix";
		case B_CPU_VENDOR_IDT:		return "IDT";
		case B_CPU_VENDOR_INTEL:	return "Intel";
		case B_CPU_VENDOR_NATIONAL_SEMICONDUCTOR:
									return "National Semiconductor";
		case B_CPU_VENDOR_RISE:		return "RISE";
		case B_CPU_VENDOR_TRANSMETA:	return "Transmeta";
		case B_CPU_VENDOR_VIA:		return "VIA";
		case B_CPU_VENDOR_IBM:		return "IBM";
		case B_CPU_VENDOR_MOTOROLA:	return "Motorola";
		case B_CPU_VENDOR_NEC:		return "NEC";
		case B_CPU_VENDOR_HYGON:	return "Hygon";
		case B_CPU_VENDOR_SUN:		return "Sun";
		case B_CPU_VENDOR_FUJITSU:	return "Fujitsu";
		default:					return NULL;
	}
}

// Fills in a human readable CPU name and the nominal clock speed in Hz.
// Either value may end up empty/zero when the system does not report it.
static void
getCPUIdentification(const char** name, uint64* frequency)
{
	enum cpu_platform		platform = B_CPU_UNKNOWN;
	enum cpu_vendor			vendor = B_CPU_VENDOR_UNKNOWN;
	uint64					defaultFrequency = 0;
	uint32					nodeCount = 0;

	// Same two call pattern the BeOS compatibility layer uses: first ask
	// for the number of topology nodes, then fill the array.
	if(get_cpu_topology_info(NULL, &nodeCount) == B_OK && nodeCount > 0)
	{
		cpu_topology_node_info* nodes = new cpu_topology_node_info[nodeCount];
		if(nodes != NULL && get_cpu_topology_info(nodes, &nodeCount) == B_OK)
		{
			for(uint32 i = 0; i < nodeCount; i++)
			{
				switch(nodes[i].type)
				{
					case B_TOPOLOGY_ROOT:
						platform = nodes[i].data.root.platform;
						break;
					case B_TOPOLOGY_PACKAGE:
						vendor = nodes[i].data.package.vendor;
						break;
					case B_TOPOLOGY_CORE:
						if(defaultFrequency == 0)
							defaultFrequency = nodes[i].data.core.default_frequency;
						break;
					default:
						break;
				}
			}
		}
		delete[] nodes;
	}

	const char* cpuName = cpuVendorName(vendor);
	if(cpuName == NULL)
		cpuName = cpuPlatformName(platform);

	*name = cpuName;
	*frequency = defaultFrequency;
}

BBitmap* AnalogView::makePanel(BRect r)
{
	system_info		sysInfo;
	BRect			rt;
	short			i;
	float			penSize;
	float			width;
	char			buf[500];
	uint64			cpu_speed;
	const char*		processor;
	BFont			font;

	get_system_info(&sysInfo);	

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
	view->StrokeArc(rt, scaleAngle(81), scaleAngle(100) - scaleAngle(80));
	
	view->SetPenSize(1);
	view->SetHighColor(kForeColor);
	for(i = 0; i <= 100; i = i + 10)
	{
		rt = computeTick(i, param.meterCenter, param.panelLength, kTickStart, kTickEnd);
		view->StrokeLine(rt.RightBottom(), rt.LeftTop());	
	}
	for(i = 0; i <= 100; i = i + 5)
	{
		rt = computeTick(i, param.meterCenter, param.panelLength, kTickStart, kTickMiddle);
		view->StrokeLine(rt.RightBottom(), rt.LeftTop());	
	}

	font.SetFamilyAndStyle("Baskerville MT Italic", 0);
	view->SetFont(&font, 0);
	view->SetFontSize(9);
	
	for(i = 0; i <= 100; i = i + 10)
	{
		rt = computeTick(i, param.meterCenter, param.panelLength+5, kTickStart, kTickEnd);
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


	font.SetFamilyAndStyle("Dutch801 Rm BT", "Bold");
	view->SetFont(&font, B_FONT_ALL);
	view->SetFontSize(12);
	strcpy(buf, "CPU [%]");
	view->MovePenTo(param.meterCenter.x - view->StringWidth(buf)/ 2, 75);
	view->DrawString(buf);
	
	if((uint32)(param.cpu + 1) == sysInfo.cpu_count)
	{

		font.SetFamilyAndStyle("Swis721 BT", "Roman");
		view->SetFont(&font);
		view->SetFontSize(9);
		getCPUIdentification(&processor, &cpu_speed);

		if(cpu_speed > 0)
		{
			int target = (int)(cpu_speed / 1000000);
			int frac = target % 100;
			int delta = -frac;
			int freqs[] = { 100, 50, 25, 75, 33, 67, 20, 40, 60, 80, 10, 30, 70, 90 };

			for (size_t x = 0; x < sizeof(freqs) / sizeof(freqs[0]); x++) {
				int ndelta = freqs[x] - frac;
				if (abs(ndelta) < abs(delta)) {
					delta = ndelta;
				}
			}
			sprintf(buf, "%dMHz", target + delta);
			view->MovePenTo(rt.right - view->StringWidth(processor) - view->StringWidth(buf) - 5, 12);
			view->DrawString(processor);
			view->MovePenTo(rt.right - view->StringWidth(buf) - 3, 12);
			view->DrawString(buf);
		}
		else
		{
			// No clock speed reported: just name the CPU.
			view->MovePenTo(rt.right - view->StringWidth(processor) - 3, 12);
			view->DrawString(processor);
		}
	}
	
	view->EndPicture();
	view->DrawPicture(picture, BPoint(0,0));
	view->Sync();
	panel->Unlock();

	delete picture;

	return panel;
}