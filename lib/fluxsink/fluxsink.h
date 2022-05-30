#ifndef FLUXSINK_H
#define FLUXSINK_H

#include "flags.h"
#include <ostream>

class Fluxmap;
class FluxSinkProto;
class HardwareFluxSinkProto;
class AuFluxSinkProto;
class VcdFluxSinkProto;
class ScpFluxSinkProto;
class Fl2FluxSinkProto;

class FluxSink
{
public:
    virtual ~FluxSink() {}

    static std::unique_ptr<FluxSink> createHardwareFluxSink(const HardwareFluxSinkProto& config);
    static std::unique_ptr<FluxSink> createAuFluxSink(const AuFluxSinkProto& config);
    static std::unique_ptr<FluxSink> createVcdFluxSink(const VcdFluxSinkProto& config);
    static std::unique_ptr<FluxSink> createScpFluxSink(const ScpFluxSinkProto& config);
    static std::unique_ptr<FluxSink> createFl2FluxSink(const Fl2FluxSinkProto& config);

    static std::unique_ptr<FluxSink> createFl2FluxSink(const std::string& filename);

    static std::unique_ptr<FluxSink> create(const FluxSinkProto& config);
	static void updateConfigForFilename(FluxSinkProto* proto, const std::string& filename);

public:
    virtual void writeFlux(int track, int side, const Fluxmap& fluxmap) = 0;

	virtual operator std::string () const = 0;
};

inline std::ostream& operator << (std::ostream& stream, FluxSink& flushSink)
{
	stream << (std::string)flushSink;
	return stream;
}

#endif

