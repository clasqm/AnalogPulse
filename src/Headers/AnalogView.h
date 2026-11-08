#ifndef ANALOGVIEW_H
#define ANALOGVIEW_H

#include "GaugeView.h"

// The CPU load pane: the shared gauge fed by per-CPU busy time.
class _EXPORT AnalogView : public GaugeView {

	public:
						AnalogView(BRect, int);
						AnalogView(BMessage *data);

static	AnalogView*		Instantiate(BMessage* data);

	protected:
	virtual	short		SampleValue();
	virtual	const char*	ClassName() const;
	virtual	const char*	AddOnSignature() const;
	virtual	const char*	AppName() const;
	virtual	void		Title(char *buf, size_t size);
	virtual	void		Identification(char *name, size_t nameSize,
							char *extra, size_t extraSize);

	private:
			double		oldActiveTime;
			double		oldSystemTime;
};

#endif // ANALOGVIEW_H
