#ifndef pVIEW_H
#include "AnalogView.h"
#endif

#include <Picture.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Modern Intel and AMD model names only exist in the brand string of the
// processor itself (CPUID leaves 0x80000002 - 0x80000004).  GCC provides
// <cpuid.h> for that; without the header we keep the vendor name instead.
#if (defined(__i386__) || defined(__x86_64__)) && __has_include(<cpuid.h>)
#	include <cpuid.h>
#	define AP_HAVE_CPUID 1
#endif


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

#ifdef AP_HAVE_CPUID
// Reads the 48 byte processor brand string (CPUID 0x80000002 - 0x80000004),
// for example "Intel(R) Core(TM) i7-12700K CPU @ 2.10GHz" or
// "AMD Ryzen 9 5900X 12-Core Processor".  Returns false when the processor
// reports none.
static bool
cpuBrandString(char* out, size_t outSize)
{
	if(__get_cpuid_max(0x80000000u, NULL) < 0x80000004u)
		return false;

	char	brand[49];
	for(unsigned leaf = 0x80000002u; leaf <= 0x80000004u; leaf++)
	{
		unsigned	regs[4] = { 0, 0, 0, 0 };
		__cpuid(leaf, regs[0], regs[1], regs[2], regs[3]);
		memcpy(brand + (leaf - 0x80000002u) * 16, regs, sizeof(regs));
	}
	brand[48] = '\0';

	// vendors blank pad the string on both ends
	char* first = brand;
	while(*first == ' ')
		first++;
	char* last = first + strlen(first);
	while(last > first && last[-1] == ' ')
		last--;
	*last = '\0';

	if(*first == '\0')
		return false;

	snprintf(out, outSize, "%s", first);
	return true;
}

// Drops the vendor's decoration so the model name fits on the panel:
// "Intel(R) Core(TM) i7-12700K CPU @ 2.10GHz" -> "Intel Core i7-12700K"
// "AMD Ryzen 9 5900X 12-Core Processor"       -> "AMD Ryzen 9 5900X"
static void
cpuModelName(const char* brand, char* out, size_t outSize)
{
	const char*	p = brand;
	size_t		used = 0;

	out[0] = '\0';
	while(*p != '\0')
	{
		while(*p == ' ' || *p == '\t')
			p++;

		const char* word = p;
		while(*p != '\0' && *p != ' ' && *p != '\t')
			p++;

		size_t length = (size_t)(p - word);
		if(length == 0)
			break;
		// the clock after '@' is already shown by the meter, "with ..."
		// only appends the GPU of the processor
		if(word[0] == '@')
			break;
		if(length == 4 && memcmp(word, "with", 4) == 0)
			break;

		bool decoration = (length == 3 && memcmp(word, "(R)", 3) == 0)
			|| (length == 3 && memcmp(word, "(r)", 3) == 0)
			|| (length == 4 && memcmp(word, "(TM)", 4) == 0)
			|| (length == 4 && memcmp(word, "(tm)", 4) == 0)
			|| (length == 3 && memcmp(word, "CPU", 3) == 0)
			|| (length == 9 && memcmp(word, "Processor", 9) == 0)
			|| (length > 5 && memcmp(word + length - 5, "-Core", 5) == 0);
		if(decoration)
			continue;

		if(used + length + 2 > outSize)
			break;
		if(used != 0)
			out[used++] = ' ';
		memcpy(out + used, word, length);
		used += length;
		out[used] = '\0';
	}
}
#endif // AP_HAVE_CPUID

// The name and the clock share the top line of the panel, so cut the name
// down until both fit.
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

// Fills in a human readable CPU name and the nominal clock speed in Hz.
// Either value may end up empty/zero when the system does not report it.
static void
getCPUIdentification(char* name, size_t nameSize, uint64* frequency)
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

	name[0] = '\0';
#ifdef AP_HAVE_CPUID
	// recent Intel and AMD processors are recognised by their model name
	char brand[64];
	if(cpuBrandString(brand, sizeof(brand)))
		cpuModelName(brand, name, nameSize);
#endif
	if(name[0] == '\0')
		snprintf(name, nameSize, "%s", cpuName);

	*frequency = defaultFrequency;
}

BBitmap* AnalogView::makePanel(BRect r)
{
	BRect			rt;
	short			i;
	float			penSize;
	float			width;
	char			buf[500];
	uint64			cpu_speed;
	char			processor[64];
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

	font.SetFamilyAndStyle("Noto Serif", "Italic");
	view->SetFont(&font, B_FONT_ALL);
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


	font.SetFamilyAndStyle("Noto Serif", "Bold");
	view->SetFont(&font, B_FONT_ALL);
	view->SetFontSize(12);
	sprintf(buf, "CPU [%d]", param.cpu);
	view->MovePenTo(param.meterCenter.x - view->StringWidth(buf)/ 2, 75 + kGaugeGap);
	view->DrawString(buf);
	
	// Every panel identifies its own processor, centred along the top.
	font.SetFamilyAndStyle("Noto Sans", "Regular");
	view->SetFont(&font);
	view->SetFontSize(9);
	getCPUIdentification(processor, sizeof(processor), &cpu_speed);

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
		clipToWidth(view, processor, rt.Width() - view->StringWidth(buf) - 8);
		float nameClockGap = 4;
		float lineWidth = view->StringWidth(processor) + nameClockGap
			+ view->StringWidth(buf);
		float x = rt.left + (rt.Width() - lineWidth) / 2;
		view->MovePenTo(x, 12);
		view->DrawString(processor);
		view->MovePenTo(x + view->StringWidth(processor) + nameClockGap, 12);
		view->DrawString(buf);
	}
	else
	{
		// No clock speed reported: just name the CPU.
		clipToWidth(view, processor, rt.Width() - 6);
		view->MovePenTo(rt.left + (rt.Width() - view->StringWidth(processor)) / 2, 12);
		view->DrawString(processor);
	}
	
	view->EndPicture();
	view->DrawPicture(picture, BPoint(0,0));
	view->Sync();
	panel->Unlock();

	delete picture;

	return panel;
}