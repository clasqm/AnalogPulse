#ifndef GAUGEVIEW_H
#define GAUGEVIEW_H

#define kSCALE_DEGREES	150.0
#define	kTickStart		0.9
#define	kTickMiddle		0.95
#define	kTickEnd		1.0
#define kPI				3.141592653589
// The panel is 5 pixels taller than it used to be (kMeterHeight in
// AnalogWindow.h), and all of that extra height goes between the
// identification line at the top and the "50" label of the gauge, which is
// therefore drawn this much lower down.
#define kGaugeGap		5.0

#include <stddef.h>

#include <Point.h>
#include <View.h>
#include <Bitmap.h>

typedef struct 
{
	BPoint	arry;
} AngleArry;


typedef struct
{
	int			cpu;
	float		panelLength;
	float		needle;
	BPoint		meterCenter;
} panelParam;

// A semicircular 0..100 meter, drawn once into a bitmap and animated by a
// needle.  The subclasses only say what the number is and what the two lines
// of text above it read, so AnalogPulse, AnalogRAM and AnalogDisk share all of
// the drawing and pulse code.
class _EXPORT GaugeView : public BView {

	public:
						GaugeView(BRect, int id);
						GaugeView(BMessage *data);
	virtual				~GaugeView();

	virtual	status_t	Archive(BMessage *data, bool deep = true) const;

			void		InitObject(BRect rt, int id, bool withDragger);

	virtual	void		Draw(BRect);
	virtual	void		Pulse();
	virtual	void		MessageReceived(BMessage *message);

			void		UpdateNeedle(short);

	protected:
		// The percentage shown by the needle, sampled on every pulse.
	virtual	short		SampleValue() = 0;

		// Identity written into the replicant archive.  Tracker loads the
		// add-on, then looks up the symbol <ClassName>::Instantiate.
	virtual	const char*	ClassName() const = 0;
	virtual	const char*	AddOnSignature() const = 0;

		// Shown in the About alert.
	virtual	const char*	AppName() const = 0;

		// Centred line above the needle pivot, for example "CPU [0]".
	virtual	void		Title(char *buf, size_t size) = 0;

		// Identification along the top of the panel.  Leave extra empty
		// when there is nothing to put beside the name.
	virtual	void		Identification(char *name, size_t nameSize,
							char *extra, size_t extraSize) = 0;

			int			Id() const { return param.cpu; }

	private:
			BRect		ComputeTick(float vu, BPoint center, long length,
							float start, float stop);
			BBitmap*	MakePanel(BRect);
			void		MakeTable(void);
			float		ScaleAngle(float);

			panelParam	param;
			BBitmap		*panel;
			BView		*view;
			AngleArry	angle[101];
			short		oldCPUTime;

	typedef	BView _inherited;
};

#endif // GAUGEVIEW_H
