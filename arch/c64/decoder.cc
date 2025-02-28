#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "protocol.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "c64.h"
#include "crc.h"
#include "bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <algorithm>

const FluxPattern SECTOR_RECORD_PATTERN(20, C64_SECTOR_RECORD);
const FluxPattern DATA_RECORD_PATTERN(20, C64_DATA_RECORD);
const FluxMatchers ANY_RECORD_PATTERN(
    {&SECTOR_RECORD_PATTERN, &DATA_RECORD_PATTERN});

static int decode_data_gcr(uint8_t gcr)
{
    switch (gcr)
    {
#define GCR_ENTRY(gcr, data) \
    case gcr:                \
        return data;
#include "data_gcr.h"
#undef GCR_ENTRY
    }
    return -1;
}

static Bytes decode(const std::vector<bool>& bits)
{
    Bytes output;
    ByteWriter bw(output);
    BitWriter bitw(bw);

    auto ii = bits.begin();
    while (ii != bits.end())
    {
        uint8_t inputfifo = 0;
        for (size_t i = 0; i < 5; i++)
        {
            if (ii == bits.end())
                break;
            inputfifo = (inputfifo << 1) | *ii++;
        }

        bitw.push(decode_data_gcr(inputfifo), 4);
    }
    bitw.flush();

    return output;
}

class Commodore64Decoder : public AbstractDecoder
{
public:
    Commodore64Decoder(const DecoderProto& config): AbstractDecoder(config) {}

    nanoseconds_t advanceToNextRecord() override
    {
        return seekToPattern(ANY_RECORD_PATTERN);
    }

    void decodeSectorRecord() override
    {
        if (readRaw20() != C64_SECTOR_RECORD)
            return;

        const auto& bits = readRawBits(5 * 10);
        const auto& bytes = decode(bits).slice(0, 5);

        uint8_t checksum = bytes[0];
        _sector->logicalSector = bytes[1];
        _sector->logicalSide = 0;
        _sector->logicalTrack = bytes[2] - 1;
        if (checksum == xorBytes(bytes.slice(1, 4)))
            _sector->status =
                Sector::DATA_MISSING; /* unintuitive but correct */
    }

    void decodeDataRecord() override
    {
        if (readRaw20() != C64_DATA_RECORD)
            return;

        const auto& bits = readRawBits(259 * 10);
        const auto& bytes = decode(bits).slice(0, 259);

        _sector->data = bytes.slice(0, C64_SECTOR_LENGTH);
        uint8_t gotChecksum = xorBytes(_sector->data);
        uint8_t wantChecksum = bytes[256];
        _sector->status =
            (wantChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
    }

    std::set<unsigned> requiredSectors(const Location& location) const override
    {
        unsigned count = sectorsForC64Track(location.logicalTrack);
        std::set<unsigned> sectors;
        for (int sectorId = 0; sectorId < count; sectorId++)
            sectors.insert(sectorId);
        return sectors;
    }
};

std::unique_ptr<AbstractDecoder> createCommodore64Decoder(
    const DecoderProto& config)
{
    return std::unique_ptr<AbstractDecoder>(new Commodore64Decoder(config));
}
