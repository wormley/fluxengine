#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "bytes.h"
#include "protocol.h"
#include "fluxsink/fluxsink.h"
#include "decoders/fluxmapreader.h"
#include "lib/fluxsink/fluxsink.pb.h"
#include "proto.h"
#include "fmt/format.h"
#include "lib/fl2.pb.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

class Fl2FluxSink : public FluxSink
{
public:
	Fl2FluxSink(const Fl2FluxSinkProto& lconfig):
		Fl2FluxSink(lconfig.filename())
	{
	}

	Fl2FluxSink(const std::string& filename):
		_filename(filename),
		_of(_filename, std::ios::out | std::ios::binary)
	{
		if (!_of.is_open())
			Error() << "cannot open output file";
	}

	~Fl2FluxSink()
	{
		FluxFileProto proto;
		proto.set_magic(FluxMagic::MAGIC);
		proto.set_version(FluxFileVersion::VERSION_2);
		for (const auto& e : _data)
		{
			auto track = proto.add_track();
			track->set_track(e.first.first);
			track->set_head(e.first.second);
			for (const auto& fluxBytes : e.second)
				track->add_flux(fluxBytes);
		}

		if (!proto.SerializeToOstream(&_of))
			Error() << "unable to write output file";
		_of.close();
		if (_of.fail())
			Error() << "FL2 write I/O error: " << strerror(errno);
	}

public:
	void writeFlux(int track, int head, const Fluxmap& fluxmap) override
	{
		auto& vector = _data[std::make_pair(track, head)];
		vector.push_back(fluxmap.rawBytes());
	}

	operator std::string () const
	{
		return fmt::format("fl2({})", _filename);
	}

private:
	std::string _filename;
	std::ofstream _of;
	std::map<std::pair<unsigned, unsigned>, std::vector<Bytes>> _data;
};

std::unique_ptr<FluxSink> FluxSink::createFl2FluxSink(const Fl2FluxSinkProto& config)
{
    return std::unique_ptr<FluxSink>(new Fl2FluxSink(config));
}

std::unique_ptr<FluxSink> FluxSink::createFl2FluxSink(const std::string& filename)
{
    return std::unique_ptr<FluxSink>(new Fl2FluxSink(filename));
}


