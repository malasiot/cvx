#ifndef DEM_HPP
#define DEM_HPP

#include <string>
#include <map>
#include <memory>

class DEM {
public:
    DEM(const std::string &folder): dem_folder_(folder) {}

    int16_t getElevation(double lat, double lon, bool bilinear = true) ;

private:

    struct HGTData {
        std::unique_ptr<uint8_t []> data_ ;

        int16_t getElevation(double lat, double lon) ;
        int16_t getElevationBilinear(double lat, double lon) ;
        bool read(const std::string &path) ;
        int16_t readPixel(int row, int col) ;
    };

    std::string getFileName(double lat, double lon) ;
    int16_t readElevationFromFile(const std::string &file_name, double lat, double lon, bool bilinear) ;

    std::map<std::string, HGTData> cache_ ;
    std::string dem_folder_ ;
};

#endif // DEM_HPP
