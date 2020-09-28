#pragma once

#include <cvx/viz/scene/scene_fwd.hpp>
#include <cvx/viz/scene/camera.hpp>
#include <cvx/viz/renderer/renderer.hpp>
#include <Eigen/Geometry>
#include <memory>

namespace cvx { namespace viz {

class CollisionObject ;

class Sensor {
public:

    // called by physics thread
    void update(float ts) ;

protected:

    // override to perform the measurement
    virtual void doUpdate() = 0;

    Eigen::Isometry3f getWorldTransform() const ;

  private:

    // determine whether we need to update based on the given update interval
    bool needsUpdate(float ts) ;


    float update_interval_ = 0.f, last_update_ = 0.f ;
    bool is_active_ = true ;
    Eigen::Isometry3f pose_ = Eigen::Isometry3f::Identity() ;
    CollisionObject *parent_ = nullptr ;
};

using SensorPtr = std::shared_ptr<Sensor> ;


class CameraSensor: public Sensor {
public:
    CameraSensor(CameraPtr camera, NodePtr scene) ;

    cv::Mat getImage() ;

protected:

    virtual void doUpdate() override ;

private:

    cv::Mat image_ ;
    CameraPtr camera_ ;
    NodePtr scene_ ;
    std::unique_ptr<OffscreenRenderer> renderer_ ;
};



}}
