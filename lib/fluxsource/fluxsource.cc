#include "globals.h"
#include "flags.h"
#include "fluxsource/fluxsource.h"
#include "fluxmap.h"
#include "lib/config.pb.h"
#include "proto.h"
#include "utils.h"
#include "fmt/format.h"
#include <regex>

static bool ends_with(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::unique_ptr<FluxSource> FluxSource::create(const FluxSourceProto& config)
{
	switch (config.source_case())
	{
		case FluxSourceProto::kDrive:
			return createHardwareFluxSource(config.drive());

		case FluxSourceProto::kErase:
			return createEraseFluxSource(config.erase());

		case FluxSourceProto::kKryoflux:
			return createKryofluxFluxSource(config.kryoflux());

		case FluxSourceProto::kTestPattern:
			return createTestPatternFluxSource(config.test_pattern());

		case FluxSourceProto::kScp:
			return createScpFluxSource(config.scp());

		case FluxSourceProto::kCwf:
			return createCwfFluxSource(config.cwf());

		case FluxSourceProto::kFl2:
			return createFl2FluxSource(config.fl2());

		default:
			Error() << "bad input disk configuration";
			return std::unique_ptr<FluxSource>();
	}
}

void FluxSource::updateConfigForFilename(FluxSourceProto* proto, const std::string& filename)
{

	static const std::vector<std::pair<std::regex, std::function<void(const std::string&, FluxSourceProto*)>>> formats =
	{
		{ std::regex("^(.*\\.flux)$"),     [](auto& s, auto* proto) { proto->mutable_fl2()->set_filename(s); }},
		{ std::regex("^(.*\\.scp)$"),      [](auto& s, auto* proto) { proto->mutable_scp()->set_filename(s); }},
		{ std::regex("^(.*\\.cwf)$"),      [](auto& s, auto* proto) { proto->mutable_cwf()->set_filename(s); }},
		{ std::regex("^erase:$"),          [](auto& s, auto* proto) { proto->mutable_erase(); }},
		{ std::regex("^kryoflux:(.*)$"),   [](auto& s, auto* proto) { proto->mutable_kryoflux()->set_directory(s); }},
		{ std::regex("^testpattern:(.*)"), [](auto& s, auto* proto) { proto->mutable_test_pattern(); }},
		{ std::regex("^drive:(.*)"),       [](auto& s, auto* proto) { proto->mutable_drive(); config.mutable_drive()->set_drive(std::stoi(s)); }},
	};

	for (const auto& it : formats)
	{
		std::smatch match;
		if (std::regex_match(filename, match, it.first))
		{
			it.second(match[1], proto);
			return;
		}
	}

	Error() << fmt::format("unrecognised flux filename '{}'", filename);
}

class TrivialFluxSourceIterator : public FluxSourceIterator
{
public:
	TrivialFluxSourceIterator(TrivialFluxSource* fluxSource, int track, int head):
		_fluxSource(fluxSource),
		_track(track),
		_head(head)
	{}

	bool hasNext() const override
	{
		return !!_fluxSource;
	}

	std::unique_ptr<const Fluxmap> next() override
	{
		auto fluxmap = _fluxSource->readSingleFlux(_track, _head);
		_fluxSource = nullptr;
		return fluxmap;
	}

private:
	TrivialFluxSource* _fluxSource;
	int _track;
	int _head;
};

std::unique_ptr<FluxSourceIterator> TrivialFluxSource::readFlux(int track, int head)
{
	return std::make_unique<TrivialFluxSourceIterator>(this, track, head);
}


