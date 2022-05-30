#ifndef VISUALISATION_H
#define VISUALISATION_H

#include <memory>
#include <map>
#include <wx/control.h>

class Sector;
class DiskFlux;
class TrackFlux;

enum {
	VISMODE_NOTHING,
	VISMODE_READING,
	VISMODE_WRITING
};

class VisualisationControl : public wxWindow
{
public:
    VisualisationControl(wxWindow* parent,
        wxWindowID winid,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0);

public:
	void Clear();
	void SetMode(int head, int track, int mode);
	void SetTrackData(std::shared_ptr<const TrackFlux> track);
	void SetDiskData(std::shared_ptr<const DiskFlux> disk);

private:
	void OnPaint(wxPaintEvent & evt);

private:
	typedef std::pair<unsigned, unsigned> key_t;

	int _head;
	int _track;
	int _mode = VISMODE_NOTHING;
	std::multimap<key_t, std::shared_ptr<const Sector>> _sectors;
    wxDECLARE_EVENT_TABLE();
};

#endif
