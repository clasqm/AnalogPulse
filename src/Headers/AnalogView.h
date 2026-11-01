#ifndef pVIEW_H
#define pVIEW_H
#endif

#define kSCALE_DEGREES	150.0
#define	kTickStart		0.9
#define	kTickMiddle		0.95
#define	kTickEnd		1.0
#define kPI				3.141592653589
// The panel is 5 pixels taller than it used to be (kMeterHeight in
// AnalogWindow.h), and all of that extra height goes between the CPU
// identification line at the top and the "50" label of the gauge, which is
// therefore drawn this much lower down.
#define kGaugeGap		5.0

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

class _EXPORT AnalogView : public BView {

	public:
						AnalogView(BRect, int);
		virtual			~AnalogView();
						AnalogView(BMessage *data);
static	AnalogView*		Instantiate(BMessage* data);
virtual	status_t		Archive(BMessage *data,bool deep = true)const;
				void	InitObject(BRect rt, int cpu);
		virtual	void	Draw(BRect);
		virtual	void	Pulse();
				void	UpdateNeedle(short);
		virtual void	MessageReceived(BMessage *message);
	private:
		panelParam		param;
		BBitmap			*panel;
		BView			*view;
		BRect			computeTick(float, BPoint, long, float, float);
		void			makeTable(void);
		float 			scaleAngle(float);
		BBitmap 		*makePanel(BRect);
		AngleArry		angle[101];
		double			oldActiveTime;
		double			oldSystemTime;
		short			nowCPUTime;
		short			oldCPUTime;
	typedef	BView _inherited;
};
