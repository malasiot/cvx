#include "dem.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <cmath>

#include <byteswap.h>

using namespace std ;

int16_t DEM::getElevation(double lat, double lon, bool bilinear) {
    std::string file_name = getFileName(lat, lon) ;

    auto it = cache_.find(file_name) ;
    if ( it == cache_.end() )
        return readElevationFromFile(file_name, lat, lon, bilinear) ;
    else {
        if ( bilinear )
            return (*it).second.getElevationBilinear(lat, lon);
        else
            return (*it).second.getElevation(lat, lon);
    }
}
/* Returns filename such as N27E086.hgt */

std::string DEM::getFileName(double lat, double lon) {
    string ns, ew ;

    if ( lat >= 0 ) ns = "N" ;
    else if ( lat < 0 ) ns = "S" ;

    if ( lon >= 0 ) ew = "E" ;
    else if ( lon < 0 ) ew = "W" ;

    ostringstream strm ;
    strm << ns ;
    strm << std::setfill('0') << std::setw(2) << static_cast<int>(abs(lat)) ;
    strm << ew ;
    strm << std::setfill('0') << std::setw(3) << static_cast<int>(abs(lon)) ;
    strm << ".hgt" ;

    return strm.str() ;
}

int16_t DEM::readElevationFromFile(const string &file_name, double lat, double lon, bool bilinear) {
    HGTData data ;

    if ( !data.read(dem_folder_ + '/' + file_name) ) return -1 ;

    int16_t val ;
    if ( bilinear )
        val = data.getElevationBilinear(lat, lon) ;
    else
        val = data.getElevation(lat, lon) ;


    cache_.emplace(file_name, std::move(data)) ;

    return val ;
}

const int SRTM_SIZE = 1201;
const int SRTM_SECOND_PER_PIXEL = 3 ;

int16_t DEM::HGTData::readPixel(int row, int col) {
    uint8_t *bytes = data_.get() + 2*((SRTM_SIZE - 1 - row)*SRTM_SIZE + col) ;
    int16_t entry = int16_t( 0 | (bytes[0] << 8) | (bytes[1] << 0) );

    return entry ;
}

int16_t DEM::HGTData::getElevation(double lat, double lon) {
    int row = int((lat - floor(lat)) * (SRTM_SIZE - 1)) ;
    int col = int((lon - floor(lon)) * (SRTM_SIZE - 1)) ;

    return readPixel(row, col) ;
}

int16_t DEM::HGTData::getElevationBilinear(double lat, double lon) {
    double latSec =  (lat-floor(lat)) * (SRTM_SIZE - 1);
    double lonSec = (lon - floor(lon)) * (SRTM_SIZE - 1);
    int row = int(latSec) ;
    int col = int(lonSec) ;

    int16_t height[4];

    height[2] = readPixel(row, col) ;
    height[0] = readPixel(row+1, col) ;
    height[3] = readPixel(row, col+1) ;
    height[1] = readPixel(row+1, col+1) ;

    //ratio where X lays
    double dy = fmod(latSec, 1);
    double dx = fmod(lonSec, 1);

       // Bilinear interpolation
       // h0------------h1
       // |
       // |--dx-- .
       // |       |
       // |      dy
       // |       |
       // h2------------h3
       return  int16_t(height[0] * dy * (1 - dx) +
               height[1] * dy * (dx) +
               height[2] * (1 - dy) * (1 - dx) +
               height[3] * (1 - dy) * dx);

}


bool DEM::HGTData::read(const string &path) {
    std::ifstream file(path, std::ios::binary);
    if ( !file ) return false ;

    data_.reset(new uint8_t [SRTM_SIZE * SRTM_SIZE * 2]) ;

    const int sz = SRTM_SIZE * SRTM_SIZE * 2 ;
    file.read(reinterpret_cast<char *>(data_.get()), sz) ;

    std::streamsize bytes = file.gcount();

    return bytes == sz ;

}
