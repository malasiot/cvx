#ifndef __CVX_VIZ_GEOMETRY_HPP__
#define __CVX_VIZ_GEOMETRY_HPP__

#include <cvx/viz/scene/scene_fwd.hpp>

#include <string>
#include <vector>
#include <memory>

#include <Eigen/Core>

namespace cvx { namespace viz {

// abstract geometry class

class Ray ;

namespace detail {
class MeshData ;
}

class Geometry {
public:
    Geometry() = default ;
    virtual ~Geometry() = default ;

    virtual bool intersect(const Ray &ray, float &t) const { return false  ; }

    MeshPtr getMesh() const { return mesh_ ; }

protected:

    MeshPtr mesh_ ;
};

class MeshGeometry: public Geometry {
public:
    MeshGeometry(const MeshPtr &mesh) ;

    bool intersect(const Ray &ray, float &t) const override ;
};

class BoxGeometry: public Geometry {
public:
    BoxGeometry(const Eigen::Vector3f &he): half_extents_(he) { makeMesh() ; }
    BoxGeometry(float hx, float hy, float hz): half_extents_{hx, hy, hz} { makeMesh() ; }
    ~BoxGeometry() {}

    Eigen::Vector3f halfExtents() const { return half_extents_ ; }

    bool intersect(const Ray &ray, float &t) const override ;

private:

    void makeMesh() ;

    Eigen::Vector3f half_extents_ ;
};

class SphereGeometry: public Geometry {
public:
    SphereGeometry(float radius): radius_(radius) { makeMesh() ; }

    float radius() const { return radius_ ; }

    bool intersect(const Ray &ray, float &t) const override ;

private:
    void makeMesh() ;

    float radius_ ;
};

class CylinderGeometry: public Geometry {
public:

    CylinderGeometry(float r, float h): radius_(r), height_(h) { makeMesh() ; }

    float radius() const { return radius_ ; }
    float height() const { return height_ ; }


  //  bool hit(const Ray &ray, Eigen::Vector3f &pos) const override ;

private:
    void makeMesh() ;

    float radius_, height_ ;
};

class ConeGeometry: public Geometry {
public:
    ConeGeometry(float r, float h): radius_(r), height_(h) {}

    float radius() const { return radius_ ; }
    float height() const { return height_ ; }

 //   bool hit(const Ray &ray, Eigen::Vector3f &pos) const override ;

private:
    float radius_, height_ ;
};

} // namespace viz
} // namespace cvx
#endif
