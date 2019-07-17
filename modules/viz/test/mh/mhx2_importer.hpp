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
    bool parseGeometries(JSONReader &v, const std::string &meshName) ;
    bool parseMesh(const std::string &name, JSONReader &v, const Eigen::Vector3f &of, float scale) ;
    bool parseVertexGroups(MHMesh &mesh, JSONReader &v) ;

    MHModel model_ ;

};

#endif
