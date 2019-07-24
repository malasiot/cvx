#ifndef MHX2_IMPORTER_HPP
#define MHX2_IMPORTER_HPP

#include <Eigen/Core>

#include "mh_model.hpp"
#include <cvx/util/misc/json_reader.hpp>


class Mhx2Importer {
public:
    Mhx2Importer() = default ;

    bool load(const std::string &fname, const std::string &meshName) ;

    const MHModel &getModel() const { return model_ ; }

private:

    using JSONReader = cvx::util::JSONReader ;

    bool parseSkeleton(JSONReader &r) ;
    bool parseGeometries(JSONReader &v) ;
    bool parseMesh(MHMesh &geom, JSONReader &v) ;
    bool parseVertexGroups(MHMesh &mesh, JSONReader &v) ;
    bool parseMaterials(JSONReader &reader) ;

    MHModel model_ ;

};

#endif
