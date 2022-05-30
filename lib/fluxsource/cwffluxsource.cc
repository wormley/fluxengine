#include "globals.h"
#include "fluxmap.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "fluxsource/fluxsource.h"
#include "proto.h"
#include "fmt/format.h"
#include <fstream>

struct CwfHeader
{
    char file_id[4];      // file ID - almost always "CWSF"
    uint8_t creator;      // usually just the CW_TYPE
    uint8_t file_type;    // indexed, etc.?
    uint8_t version;      // version of this file
    uint8_t clock_rate;   // clock rate used: 0, 1, 2 (2 = 28MHz)
    uint8_t drive_type;   // type of drive
    uint8_t tracks;    // number of tracks
    uint8_t sides;        // number of sides
    uint8_t index_mark;   // nonzero if index marks are included
    uint8_t step;         // track stepping interval
    uint8_t filler[15];   // reserved for expansion
    uint8_t comment[100]; // brief comment
};

struct CwfTrack
{
    uint8_t track;     // sequential
    uint8_t side;
    uint8_t unused[2];
    uint8_t length[4]; // little-endian
};

class CwfFluxSource : public TrivialFluxSource
{
public:
    CwfFluxSource(const CwfFluxSourceProto& config):
		_config(config)
    {
		_if.open(_config.filename(), std::ios::in | std::ios::binary);
		if (!_if.is_open())
			Error() << fmt::format("cannot open input file '{}': {}",
				_config.filename(), strerror(errno));

		_if.read((char*) &_header, sizeof(_header));
		check_for_error();

		if ((_header.file_id[0] != 'C')
				|| (_header.file_id[1] != 'W')
				|| (_header.file_id[2] != 'S')
				|| (_header.file_id[3] != 'F'))
			Error() << "input not a CWF file";

		switch (_header.clock_rate)
		{
			case 1: _clockRate = 14161000.0; break;
			case 2: _clockRate = 28322000.0; break;
			default:
				Error() << "unsupported clock rate";
		}

		std::cout << fmt::format("CWF {}x{} = {} tracks, {} sides\n",
			_header.tracks, _header.step, _header.tracks*_header.step, _header.sides);
		std::cout << fmt::format("CWF sample clock rate: {} MHz\n", _clockRate / 1e6);

		int tracks = _header.tracks*_header.sides;
		for (int i=0; i<tracks; i++)
		{
			CwfTrack trackheader;
			_if.read((char*) &trackheader, sizeof(trackheader));
			check_for_error();

			uint32_t length = Bytes(trackheader.length, 4).reader().read_le32() - sizeof(CwfTrack);
			unsigned track_number = trackheader.track * _header.step;

			off_t pos = _if.tellg();
			_trackOffsets[std::make_pair(track_number, trackheader.side)] = std::make_pair(pos, length);
			_if.seekg(pos + length);
		}
	}

public:
    std::unique_ptr<const Fluxmap> readSingleFlux(int track, int side)
    {
		const auto& p = _trackOffsets.find(std::make_pair(track, side));
		if (p == _trackOffsets.end())
			return std::make_unique<const Fluxmap>();

		off_t pos = p->second.first;;
		size_t length = p->second.second;
		_if.seekg(pos);

		std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
		uint32_t pending = 0;
		bool oldindex = true;
		for (size_t cursor = 0; cursor < length; cursor++)
		{
			uint32_t b = _if.get();
			bool index = !!(b & 0x80);
			b &= 0x7f;
			if (b == 0x7f)
			{
				pending += 0x7f;
				continue;
			}
			b += pending;
			pending = 0;

			double interval_us = b * (1e6/_clockRate);
			fluxmap->appendInterval(interval_us / US_PER_TICK);
			fluxmap->appendPulse();

			if (index && !oldindex)
				fluxmap->appendIndex();
			oldindex = index;
		}
		check_for_error();

		return fluxmap;
    }

    void recalibrate() {}

private:
	void check_for_error()
	{
		if (_if.fail())
			Error() << fmt::format("SCP read I/O error: {}", strerror(errno));
	}

private:
    const CwfFluxSourceProto& _config;
	std::ifstream _if;
	CwfHeader _header;
	nanoseconds_t _clockRate;
	std::map<std::pair<int, int>, std::pair<off_t, size_t>> _trackOffsets;
};

std::unique_ptr<FluxSource> FluxSource::createCwfFluxSource(const CwfFluxSourceProto& config)
{
    return std::unique_ptr<FluxSource>(new CwfFluxSource(config));
}
