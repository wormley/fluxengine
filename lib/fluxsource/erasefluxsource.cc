#include "globals.h"
#include "fluxmap.h"
#include "fluxsource/fluxsource.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "fmt/format.h"

class EraseFluxSource : public TrivialFluxSource
{
public:
    EraseFluxSource(const EraseFluxSourceProto& config) {}
    ~EraseFluxSource() {}

public:
    std::unique_ptr<const Fluxmap> readSingleFlux(int track, int side) override
    {
		return std::unique_ptr<const Fluxmap>();
    }

    void recalibrate() {}
};

std::unique_ptr<FluxSource> FluxSource::createEraseFluxSource(const EraseFluxSourceProto& config)
{
    return std::make_unique<EraseFluxSource>(config);
}



