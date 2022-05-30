#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include "decoders/decoders.h"
#include "image.h"
#include "logger.h"
#include "arch/northstar/northstar.h"
#include "lib/imagewriter/imagewriter.pb.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class NsiImageWriter : public ImageWriter
{
public:
	NsiImageWriter(const ImageWriterProto& config):
		ImageWriter(config)
	{}

	void writeImage(const Image& image)
	{
		const Geometry& geometry = image.getGeometry();
		bool mixedDensity = false;

		size_t trackSize = geometry.numSectors * geometry.sectorSize;

		if (geometry.numTracks * trackSize == 0) {
			Logger() << "No sectors in output; skipping .nsi image file generation.";
			return;
		}

		Logger() << fmt::format("Writing {} tracks, {} sides, {} sectors, {} ({} bytes/sector), {} kB total",
				geometry.numTracks, geometry.numSides,
				geometry.numSectors, geometry.sectorSize == 256 ? "SD" : "DD", geometry.sectorSize,
				geometry.numTracks * geometry.numSides * geometry.numSectors * geometry.sectorSize / 1024);

		std::ofstream outputFile(_config.filename(), std::ios::out | std::ios::binary);
		if (!outputFile.is_open())
			Error() << "cannot open output file";

		unsigned sectorFileOffset;
		for (int track = 0; track < geometry.numTracks * geometry.numSides; track++)
		{
			int side = (track < geometry.numTracks) ? 0 : 1;
			for (int sectorId = 0; sectorId < geometry.numSectors; sectorId++)
			{
				const auto& sector = image.get(track % geometry.numTracks, side, sectorId);
				if (sector)
				{
					if (side == 0) { /* Side 0 is from track 0-34 */
						sectorFileOffset = track * trackSize + sectorId * geometry.sectorSize;
					}
					else { /* Side 1 is from track 70-35 */
						sectorFileOffset = (geometry.sectorSize * geometry.numSectors * geometry.numTracks) + /* Skip over side 0 */
							((geometry.numTracks - 1) - (track % geometry.numTracks)) * (geometry.sectorSize * geometry.numSectors) +
							(sectorId * geometry.sectorSize); /* Sector offset from beginning of track. */
					}
					outputFile.seekp(sectorFileOffset, std::ios::beg);
					if ((geometry.sectorSize == 512) && (sector->data.size() == 256)) {
						/* North Star DOS provided an upgrade path for disks formatted as single-
						 * density to hold double-density data without reformatting.  In this
						 * case, the four directory blocks will be single-density but other areas
						 * of the disk are double-density.  This cannot be accurately represented
						 * using a .nsi file, so in these cases, we pad the sector to 512-bytes,
						 * filling with spaces.
						 */
						char fill[256];
						memset(fill, ' ', sizeof(fill));
						if (mixedDensity == false) {
							Logger() << "Warning: Disk contains mixed single/double-density sectors.";
						}
						mixedDensity = true;
						sector->data.slice(0, 256).writeTo(outputFile);
						outputFile.write(fill, sizeof(fill));
					} else {
						sector->data.slice(0, geometry.sectorSize).writeTo(outputFile);
					}
				}
			}
		}
	}
};

std::unique_ptr<ImageWriter> ImageWriter::createNsiImageWriter(
	const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new NsiImageWriter(config));
}
