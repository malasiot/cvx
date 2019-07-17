#ifndef MH_SCENE_HPP
#define MH_SCENE_HPP

#include "mh_model.hpp"

#include <cvx/viz/scene/scene.hpp>

class MHNode: public cvx::viz::Node {
public:
    MHNode(const MHModel &model) ;

private:
    void createGeometry(const MHModel &model) ;
};









#endif
