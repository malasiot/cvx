#include <cvx/viz/scene/mesh.hpp>

#include <Eigen/Geometry>

#include <algorithm>

#include "octree.hpp"

using namespace std ;
using namespace Eigen ;
using namespace cvx::util ;

namespace cvx { namespace viz {

Mesh::~Mesh()
{
    if ( octree_ ) delete octree_ ;
}

bool Mesh::isSimpleIndexed() const {
    if ( normals_.hasIndices() ) return false ;
    if ( colors_.hasIndices() ) return false ;
    for( uint t=0 ; t<MAX_TEXTURES ; t++ )
        if ( tex_coords_[t].hasIndices() ) return false ;

    return true ;
}

MeshPtr Mesh::createWireCube(const Vector3f &hs) {

    MeshPtr m(new Mesh(Lines)) ;

    m->vertices().data() =
    {{ -hs.x(), +hs.y(), +hs.z() }, { +hs.x(), +hs.y(), +hs.z() }, { +hs.x(), -hs.y(), +hs.z() }, { -hs.x(), -hs.y(), +hs.z() },
    { -hs.x(), +hs.y(), -hs.z() }, { +hs.x(), +hs.y(), -hs.z() }, { +hs.x(), -hs.y(), -hs.z() }, { -hs.x(), -hs.y(), -hs.z() } } ;
    m->vertices().indices() =  {  0, 1, 1, 2, 2, 3, 3, 0,  4, 5, 5, 6, 6, 7,  7, 4, 0, 4, 1, 5, 2, 6, 3, 7 };

    return m ;
}

MeshPtr Mesh::createSolidCube(const Vector3f &hs) {
    MeshPtr m(new Mesh(Triangles)) ;
    m->normals().data() = {{ 0.0, 0.0, 1.0 }, {0.0, 0.0, -1.0}, { 0.0, 1.0, 0.0 }, {1.0, 0.0, 0.0}, {0.0, -1.0, 0.0}, { -1.0, 0.0, 0.0}} ;
    m->vertices().data() = {{ -hs.x(), +hs.y(), +hs.z() }, { +hs.x(), +hs.y(), +hs.z() }, { +hs.x(), -hs.y(), +hs.z() }, { -hs.x(), -hs.y(), +hs.z() },
    { -hs.x(), +hs.y(), -hs.z() }, { +hs.x(), +hs.y(), -hs.z() }, { +hs.x(), -hs.y(), -hs.z() }, { -hs.x(), -hs.y(), -hs.z() } } ;

    m->vertices().indices() = {  1, 0, 3,  7, 4, 5,  4, 0, 1,  5, 1, 2,  2, 3, 7,  0, 4, 7,  1, 3, 2,  7, 5, 6,  4, 1, 5,  5, 2, 6,  2, 7, 6, 0, 7, 3};
    m->normals().indices() = {  0, 0, 0,  1, 1, 1,  2, 2, 2,  3, 3, 3,  4, 4, 4,  5, 5, 5,  0, 0, 0,  1, 1, 1,  2, 2, 2,  3, 3, 3,  4, 4, 4, 5, 5, 5};

    m->texCoords(0).data() = { { 0.0, 0.0 },  {1.0, 0.0},  {1.0, 1.0}, {0.0, 1.0} };
    m->texCoords(0).indices() = { 1, 0, 3, 1, 3, 2, 1, 0, 3, 1, 3, 2, 1, 0, 3, 1, 3, 2, 1, 0, 3, 1, 3, 2, 1, 0, 3, 1, 3, 2, 1, 0, 3, 1, 3, 2 } ;
    return m ;
}

// adapted from freeglut

static void makeCircleTable(vector<float> &sint, vector<float> &cost, int n, bool half_circle = false) {

    /* Table size, the sign of n flips the circle direction */

    const size_t size = abs(n);

    /* Determine the angle between samples */

    const float angle = (half_circle ? 1 : 2)*M_PI/(float)( ( n == 0 ) ? 1 : n );

    sint.resize(size+1) ; cost.resize(size+1) ;

    /* Compute cos and sin around the circle */

    sint[0] = 0.0;
    cost[0] = 1.0;

    for ( size_t i =1 ; i<size; i++ ) {
        sint[i] = sin(angle*i);
        cost[i] = cos(angle*i);
    }

    /* Last sample is duplicate of the first */

    sint[size] = sint[0];
    cost[size] = cost[0];
}

MeshPtr Mesh::createSolidCone(float radius, float height, size_t slices, size_t stacks)
{
    MeshPtr m(new Mesh(Triangles)) ;


    float z0,z1;
    float r0,r1;

    const float zStep = height / std::max(stacks, (size_t)1) ;
    const float rStep = radius / std::max(stacks, (size_t)1) ;

    const float cosn = ( height / sqrt ( height * height + radius * radius ));
    const float sinn = ( radius / sqrt ( height * height + radius * radius ));

    vector<float> sint, cost;
    makeCircleTable( sint, cost, slices);

    /* Cover the circular base with a triangle fan... */

    z0 = 0.0;
    z1 = z0+ zStep;

    r0 = radius ;
    r1 = r0 - rStep;

    // the base of the cone is on (0, 0, 0) aligned with the z-axis and pointing towards positive z

    m->vertices().data().push_back({0, 0, z0}) ;
    m->normals().data().push_back({0, 0, -1}) ;

    for( uint i=0 ; i<slices ; i++ ) {
        m->vertices().data().push_back({cost[i]*r0, sint[i]*r0, z0}) ;
    }

    for( uint i=0 ; i<slices ; i++ ) {

        m->vertices().indices().push_back(i+1) ;
        m->vertices().indices().push_back(0) ;
        m->vertices().indices().push_back(i == slices-1 ? 1 : i+2) ;

        m->normals().indices().push_back(0) ;
        m->normals().indices().push_back(0) ;
        m->normals().indices().push_back(0) ;
    }

    // normals shared by all side vertices

    for( uint i=0 ; i<slices ; i++ ) {
        m->normals().data().push_back({cost[i]*sinn, sint[i]*sinn, cosn}) ;
    }

    for( size_t j = 1;  j < stacks; j++ ) {

        for( uint i=0 ; i<slices ; i++ ) {
            m->vertices().data().push_back({cost[i]*r1, sint[i]*r1, z1}) ;
        }

        for( uint i=0 ; i<slices ; i++ ) {
            size_t pn = ( i == slices - 1 ) ? 0 : i+1 ;
            m->vertices().indices().push_back((j-1)*slices + i + 1) ;
            m->vertices().indices().push_back((j-1)*slices + pn + 1) ;
            m->vertices().indices().push_back((j)*slices + pn + 1) ;

            m->vertices().indices().push_back((j-1)*slices + i + 1) ;
            m->vertices().indices().push_back((j)*slices + pn + 1) ;
            m->vertices().indices().push_back((j)*slices + i + 1) ;

            m->normals().indices().push_back(i + 1) ;
            m->normals().indices().push_back(pn + 1) ;
            m->normals().indices().push_back(pn + 1) ;

            m->normals().indices().push_back(i + 1) ;
            m->normals().indices().push_back(pn + 1) ;
            m->normals().indices().push_back(i + 1) ;
        }

        z1 += zStep;
        r1 -= rStep ;
    }

    // link apex with last stack

    size_t offset = (stacks - 1)*slices + 1;

    m->vertices().data().push_back({0, 0, z1}) ;

    for( uint i=0 ; i<slices ; i++ ) {
        size_t pn = ( i == slices - 1 ) ? 0 : i+1 ;
        m->vertices().indices().push_back(offset + i) ;
        m->vertices().indices().push_back(offset + pn) ;
        m->vertices().indices().push_back(offset + slices) ;

        m->normals().indices().push_back(i + 1) ;
        m->normals().indices().push_back(pn + 1) ;
        m->normals().indices().push_back(i + 1) ;
    }

    return m ;
}

MeshPtr Mesh::createWireCone(float radius, float height, size_t slices, size_t stacks)
{
    MeshPtr m(new Mesh(Lines)) ;

    float z0,z1;
    float r0;

    vector<float> sint, cost;
    makeCircleTable( sint, cost, slices);

    z0 = 0.0;
    r0 = radius ;
    z1 = z0 + height ;

    for( uint i=0 ; i<slices ; i++ ) {
        m->vertices().data().push_back({cost[i]*r0, sint[i]*r0, z0}) ;
    }

    m->vertices().data().push_back({0, 0, z1}) ;

    for( uint i=0 ; i<slices-1 ; i++ ) {
        m->vertices().indices().push_back(i) ;
        m->vertices().indices().push_back(i+1) ;
    }
    m->vertices().indices().push_back(slices-1) ;
    m->vertices().indices().push_back(0) ;

    for( uint i=0 ; i<slices ; i++ ) {
        m->vertices().indices().push_back(i) ;
        m->vertices().indices().push_back(slices) ;
    }

    return m ;
}

MeshPtr Mesh::createSolidCylinder(float radius, float height, size_t slices, size_t stacks)
{
    MeshPtr m(new Mesh(Triangles)) ;

    float z0,z1;

    const float zStep = height / std::max(stacks, (size_t)1) ;

    vector<float> sint, cost;
    makeCircleTable( sint, cost, slices);

    /* Cover the circular base with a triangle fan... */

    z0 = -height/2.0;
    z1 = z0 + zStep;

    m->vertices().data().push_back({0, 0, z0}) ;
    m->normals().data().push_back({0, 0, -1}) ;

    for( uint i=0 ; i<slices ; i++ ) {
        m->vertices().data().push_back({cost[i]*radius, sint[i]*radius, z0}) ;
    }

    for( uint i=0 ; i<slices ; i++ ) {

        m->vertices().indices().push_back(i+1) ;
        m->vertices().indices().push_back(0) ;
        m->vertices().indices().push_back(i == slices-1 ? 1 : i+2) ;

        m->normals().indices().push_back(0) ;
        m->normals().indices().push_back(0) ;
        m->normals().indices().push_back(0) ;
    }

    // normals shared by all side vertices

    for( uint i=0 ; i<slices ; i++ ) {
        m->normals().data().push_back({cost[i], sint[i], 1.0}) ;
    }

    for( size_t j = 1;  j <= stacks; j++ ) {

        for( uint i=0 ; i<slices ; i++ ) {
            m->vertices().data().push_back({cost[i]*radius, sint[i]*radius, z1}) ;
        }

        for( uint i=0 ; i<slices ; i++ ) {
            size_t pn = ( i == slices - 1 ) ? 0 : i+1 ;
            m->vertices().indices().push_back((j-1)*slices + i + 1) ;
            m->vertices().indices().push_back((j-1)*slices + pn + 1) ;
            m->vertices().indices().push_back((j)*slices + pn + 1) ;

            m->vertices().indices().push_back((j-1)*slices + i + 1) ;
            m->vertices().indices().push_back((j)*slices + pn + 1) ;
            m->vertices().indices().push_back((j)*slices + i + 1) ;

            m->normals().indices().push_back(i + 1) ;
            m->normals().indices().push_back(pn + 1) ;
            m->normals().indices().push_back(pn + 1) ;

            m->normals().indices().push_back(i + 1) ;
            m->normals().indices().push_back(pn + 1) ;
            m->normals().indices().push_back(i + 1) ;
        }

        z1 += zStep;
    }

    // link apex with last stack

    size_t offset = (stacks)*slices + 1;

    m->vertices().data().push_back({0, 0, height/2.0}) ;
    m->normals().data().push_back({0, 0, 1}) ;

    for( uint i=0 ; i<slices ; i++ ) {
        size_t pn = ( i == slices - 1 ) ? 0 : i+1 ;
        m->vertices().indices().push_back(offset + i) ;
        m->vertices().indices().push_back(offset + pn) ;
        m->vertices().indices().push_back(offset + slices) ;

        m->normals().indices().push_back(slices+1) ;
        m->normals().indices().push_back(slices+1) ;
        m->normals().indices().push_back(slices+1) ;
    }

    return m ;
}

MeshPtr Mesh::createCapsule(float radius, float height, size_t slices, size_t head_stacks, size_t body_stacks)
{
    MeshPtr m(new Mesh(Triangles)) ;

    // make cylinder

    float z0,z1;

    const float zStep = (height - 2*radius) / std::max(body_stacks, (size_t)1) ;

    vector<float> sint, cost;
    makeCircleTable( sint, cost, slices);

    z0 = -height/2.0f - radius ;
    z1 = z0 + zStep;

    for( uint i=0 ; i<slices ; i++ ) {
        m->vertices().data().push_back({cost[i]*radius, sint[i]*radius, z0}) ;
    }

    // normals shared by all side vertices

    for( uint i=0 ; i<slices ; i++ ) {
        m->normals().data().push_back({cost[i], sint[i], 1.0}) ;
    }

    for( size_t j = 1;  j <= body_stacks; j++ ) {

        for( uint i=0 ; i<slices ; i++ ) {
            m->vertices().data().push_back({cost[i]*radius, sint[i]*radius, z1}) ;
        }

        for( uint i=0 ; i<slices ; i++ ) {
            size_t pn = ( i == slices - 1 ) ? 0 : i+1 ;
            m->vertices().indices().push_back((j-1)*slices + i) ;
            m->vertices().indices().push_back((j-1)*slices + pn) ;
            m->vertices().indices().push_back((j)*slices + pn) ;

            m->vertices().indices().push_back((j-1)*slices + i) ;
            m->vertices().indices().push_back((j)*slices + pn) ;
            m->vertices().indices().push_back((j)*slices + i) ;

            m->normals().indices().push_back(i) ;
            m->normals().indices().push_back(pn) ;
            m->normals().indices().push_back(pn) ;

            m->normals().indices().push_back(i) ;
            m->normals().indices().push_back(pn) ;
            m->normals().indices().push_back(i) ;

        }

        z1 += zStep;
    }

    return m ;

}

MeshPtr Mesh::makePointCloud(const PointList3f &pts) {

     MeshPtr m(new Mesh(Points)) ;

     m->vertices().data() = pts ;

     return m ;
}

MeshPtr Mesh::makePointCloud(const PointList3f &coords, const PointList3f &clrs) {

    MeshPtr m(new Mesh(Points)) ;

    m->vertices().data() = coords ;
    m->colors().data() = clrs ;

    return m ;

}



static Vector3f normal_triangle(const Vector3f &v1, const Vector3f &v2, const Vector3f &v3)
{
    Vector3f n1, n2 ;

    n1 = v1 - v2 ;
    n2 = v1 - v3 ;
    return  n1.cross(n2).normalized() ;

}
void Mesh::computeNormals()
{
    assert( vertices().hasIndices() ) ;

    size_t n = vertices().data().size() ;
    normals_.data().resize(n) ;

    for( int i=0 ; i<n ; i++ ) normals_.data()[i] = Vector3f::Zero() ;

    for( int i=0 ; i<vertices().indices().size() ; i+=3 )
    {
        uint idx0 = vertices().indices()[i] ;
        uint idx1 = vertices().indices()[i+1] ;
        uint idx2 = vertices().indices()[i+2] ;
        Vector3f n = normal_triangle(vertices().data()[idx0], vertices().data()[idx1], vertices().data()[idx2]) ;

        normals_.data()[idx0] += n ;
        normals_.data()[idx1] += n ;
        normals_.data()[idx2] += n ;
    }

    for( int i=0 ; i<n ; i++ ) normals_.data()[i].normalize() ;
}



void Mesh::computeBoundingBox(Vector3f &vmin, Vector3f &vmax) const {

    auto &&vertices = vertices_.data() ;
    assert( !vertices.empty() ) ;

    vmin = vertices[0] ;

    for( const Vector3f &v: vertices ) {
        vmin.x() = std::min(vmin.x(), v.x()) ;
        vmin.y() = std::min(vmin.y(), v.y()) ;
        vmin.z() = std::min(vmin.z(), v.z()) ;
        vmax.x() = std::max(vmax.x(), v.x()) ;
        vmax.y() = std::max(vmax.y(), v.y()) ;
        vmax.z() = std::max(vmax.z(), v.z()) ;
    }
}

void Mesh::makeOctree()
{
    assert( this->ptype_ == Mesh::Triangles ) ;

    octree_ = new detail::Octree(*this, 5) ;
}



bool Mesh::intersect(const Ray &ray, float &t) const
{
    if ( octree_ == nullptr ) return false ;

    uint tindex ;
    octree_->intersect(ray, tindex, t) ;

    return true ;
}
MeshPtr Mesh::createWireCylinder(float radius, float height, size_t slices, size_t stacks)
{
    MeshPtr m(new Mesh(Lines)) ;

    float z0,z1;

    const float zStep = height / std::max(stacks, (size_t)1) ;

    vector<float> sint, cost;
    makeCircleTable( sint, cost, slices);

    z0 = 0.0;

    for( uint i=0 ; i<slices ; i++ ) {
        m->vertices().data().push_back({cost[i]*radius, sint[i]*radius, z0}) ;
    }

    for( uint i=0 ; i<slices ; i++ ) {
        m->vertices().data().push_back({cost[i]*radius, sint[i]*radius, z0 + height}) ;
    }

    for( uint i=0 ; i<slices-1 ; i++ ) {
        m->vertices().indices().push_back(i) ;
        m->vertices().indices().push_back(i+1) ;
    }
    m->vertices().indices().push_back(slices-1) ;
    m->vertices().indices().push_back(0) ;

    uint offset = slices ;

    for( uint i=0 ; i<slices-1 ; i++ ) {
        m->vertices().indices().push_back(offset + i) ;
        m->vertices().indices().push_back(offset + i+1) ;
    }
    m->vertices().indices().push_back(offset + slices-1) ;
    m->vertices().indices().push_back(offset) ;

    for( uint i=0 ; i<slices ; i++ ) {

        m->vertices().indices().push_back(i) ;
        m->vertices().indices().push_back(i + offset) ;
    }

    return m ;
}


static void exportToObj(const std::string &fname, const PointList3f &vertices, const PointList3f &normals, const std::vector<uint> &indices) {
    ofstream strm(fname) ;

    for( uint i=0 ;i<vertices.size() ; i++ ) {
        strm << "v " << vertices[i].adjoint() << endl ;
    }

    for( uint i=0 ;i<normals.size() ; i++ ) {
        strm << "vn " << normals[i].adjoint() << endl ;
    }

    for( uint i=0 ; i<indices.size() ; i+=3) {
        strm << "f " << indices[i] + 1 << ' ' << indices[i+1] +1<< ' ' << indices[i+2] + 1<< endl ;
    }
}
MeshPtr Mesh::createSolidSphere(float radius, size_t slices, size_t stacks) {

    MeshPtr m(new Mesh(Triangles)) ;
    int idx = 0;
    float x,y,z;
    int n_vertices ;

    /* Pre-computed circle */
    vector<float> sint1, cost1, sint2, cost2;

    /* number of unique vertices */
    if (slices==0 || stacks<2 )  {
        return nullptr ;
    }

    n_vertices = slices*(stacks-1) + 2 ;

    makeCircleTable(sint1, cost1, -slices, false) ;
    makeCircleTable(sint2, cost2, stacks, true) ;

    m->vertices().data().resize(n_vertices) ;
    m->normals().data().resize(n_vertices) ;

    /* top */

    auto &&vertices = m->vertices().data() ;
    auto &&normals = m->normals().data() ;
    auto &&indices = m->vertices().indices() ;

    vertices[0] = { 0.f, 0.f, radius } ;
    normals[0] = { 0.f, 0.f, 1.0f } ;

    idx = 1;

    /* each stack */
    for( uint i=1; i<stacks; i++ )
    {
        for( uint j=0; j<slices; j++, idx++)
        {
            x = cost1[j]*sint2[i];
            y = sint1[j]*sint2[i];
            z = cost2[i];

            vertices[idx] = { x*radius, y*radius, z*radius } ;
            normals[idx] = { x, y, z } ;
        }
    }

    vertices[idx] = { 0.0f, 0.0f, -radius } ;
    normals[idx] = { 0.0f, 0.0f, -1.0f } ;

    indices.resize(6*slices + 6*(stacks-2)*slices) ;

    /* top stack */

    idx = 0 ;
    for ( uint j=0;  j<slices-1;  j++) {
        indices[idx++] = j+2 ;
        indices[idx++] = j+1 ;
        indices[idx++] = 0 ;
    }

    indices[idx++] = 1 ;
    indices[idx++] = slices ;
    indices[idx++] = 0 ;

    for ( uint i=0; i< stacks-2; i++ )
    {
        uint offset = 1+i*slices;                    /* triangle_strip indices start at 1 (0 is top vertex), and we advance one stack down as we go along */
        uint j ;

        for ( j=0; j<slices-1; j++ ) {
            indices[idx++] = offset + j + slices ;
            indices[idx++] = offset + j ;
            indices[idx++] = offset + j + 1 ;

            indices[idx++] = offset + j + slices ;
            indices[idx++] = offset + j + 1;
            indices[idx++] = offset + j + slices + 1 ;
        }

        indices[idx++] = offset + slices ;
        indices[idx++] = offset + j + slices ;
        indices[idx++] = offset  ;

        indices[idx++] = offset  ;
        indices[idx++] = offset + j + slices ;
        indices[idx++] = offset + j ;

    }

    /* bottom stack */
    int offset = 1+(stacks-2)*slices;               /* triangle_strip indices start at 1 (0 is top vertex), and we advance one stack down as we go along */

    for ( uint j=0;  j<slices-1;  j++) {
        indices[idx++] = j + offset  ;
        indices[idx++] = j + offset + 1;
        indices[idx++] = n_vertices-1 ;
    }

    indices[idx++] = offset + slices - 1 ;
    indices[idx++] = offset ;
    indices[idx++] = n_vertices-1 ;

//    exportToObj("/tmp/mesh.obj", vertices, normals, indices) ;

    return m ;
}

MeshPtr Mesh::flatten(const MeshPtr &src) {

    if ( src->isSimpleIndexed() ) return src ;

    // check if we need to flatten otherwise return src mesh

    const auto &src_vertices = src->vertices() ;
    const auto &src_normals = src->normals() ;
    const auto &src_colors = src->colors() ;

    MeshPtr dst(new Mesh(Triangles)) ;

    auto &dst_vertices = dst->vertices().data() ;
    auto &dst_normals = dst->normals().data() ;
    auto &dst_colors = dst->colors().data() ;

    size_t n_indices = src_vertices.indices().size() ;

    for( uint i=0 ; i<n_indices ; i++) {

        uint32_t vidx = src_vertices.indices()[i] ;
        const Vector3f &pos = src_vertices.data()[vidx] ;
        dst_vertices.push_back(pos) ;

        if ( src_normals.hasIndices() ) {
            uint32_t nidx = src_normals.indices()[i] ;
            const Vector3f &normal = src_normals.data()[nidx] ;
            dst_normals.push_back(normal) ;
        } else if ( !src_normals.data().empty() ) {
            const Vector3f &norm = src_normals.data()[vidx] ;
            dst_normals.push_back(norm) ;
        }

        if ( src_colors.hasIndices() ) {
            uint32_t cidx = src_colors.indices()[i] ;
            const Vector3f &color = src_colors.data()[cidx] ;
            dst_colors.push_back(color) ;
        } else if ( !src_colors.data().empty() ) {
            const Vector3f &color = src_colors.data()[vidx] ;
            dst_colors.push_back(color) ;
        }
/*
        for( uint t=0 ; t<MAX_TEXTURES ; t++ ) {
            auto uvs = src->texCoords(t) ;
            if ( uvs.hasIndices() ) {
                uint32_t cidx = uvs.indices()[i] ;
                const Vector2f &uv = uvs.data()[cidx] ;
                dst->texCoords(t).data().push_back(uv) ;
            } else if ( !uvs.data().empty() ) {
                const Vector2f &uv = uvs.data()[vidx] ;
                dst->texCoords(t).data().push_back(uv) ;
            }

        }
*/

    }

    return dst ;
}

} // namespace viz
} // namespace cvx
