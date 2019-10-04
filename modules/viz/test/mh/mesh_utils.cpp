#include "mesh_utils.hpp"

#include "vcg_mesh.hpp"
#include <vcg/complex/algorithms/convex_hull.h>

using namespace Eigen ;
// =============================================================================

// =============================================================================

void poisson_disk(float radius,
                  int nb_samples,
                  const std::vector<Vector3f>& verts,
                  const std::vector<Vector3f>& nors,
                  const std::vector<uint>& tris,
                  std::vector<Vector3f>& samples_pos,
                  std::vector<Vector3f>& samples_nor)
{
    assert(verts.size() == nors.size());
    assert(verts.size() > 0);
    assert(tris.size() > 0);

    vcg::MyMesh vcg_mesh, sampler;
    vcg_mesh.concat((float*)&(verts[0]), (uint*)&(tris[0]), verts.size(), tris.size()/3);
    vcg_mesh.set_normals( (float*)&(nors[0]) );
    vcg_mesh.update_bb();

    vcg::MyAlgorithms::Poison_setup pp;
    pp._radius = radius;
    pp._nb_samples = nb_samples;
    pp._approx_geodesic_dist = true;
    vcg::MyAlgorithms::poison_disk_sampling(vcg_mesh, pp, sampler);

    const int nb_vert = sampler.vert.size();
    samples_pos.clear();
    samples_nor.clear();
    samples_pos.resize( nb_vert );
    samples_nor.resize( nb_vert );
    vcg::MyMesh::VertexIterator vi = sampler.vert.begin();
    for(int i = 0; i < nb_vert; ++i, ++vi)
    {
        vcg::MyMesh::CoordType  p = (*vi).P();
        vcg::MyMesh::NormalType n = (*vi).N();
        samples_pos[i] = Vector3f(p.X(), p.Y(), p.Z());
        samples_nor[i] = Vector3f(n.X(), n.Y(), n.Z());
    }

    // sampler.get_vertices( samples_pos );

}

class CHVertex ;
class CHFace ;

struct CHUsedTypes : public vcg::UsedTypes<vcg::Use<CHVertex>   ::AsVertexType,
                                           vcg::Use<CHFace>     ::AsFaceType>{};

class CHVertex  : public vcg::Vertex< CHUsedTypes, vcg::vertex::Coord3f, vcg::vertex::Normal3f, vcg::vertex::BitFlags  >{};
class CHFace    : public vcg::Face<   CHUsedTypes, vcg::face::FFAdj,  vcg::face::VertexRef, vcg::face::BitFlags > {};
class CHMesh    : public vcg::tri::TriMesh< std::vector<CHVertex>, std::vector<CHFace> > {};


void convex_hull( const std::vector<Vector3f>& verts,
                  const std::vector<Vector3f>& nors,
                  const std::vector<uint>& tris,
        std::vector<Vector3f>& out_verts,
        std::vector<Vector3f>& out_nors,
        std::vector<uint>& out_tris )
{
    assert(verts.size() == nors.size());
    assert(verts.size() > 0);
    assert(tris.size() > 0);

    vcg::MyMesh vcg_mesh ;
    vcg_mesh.concat((float*)&(verts[0]), (uint*)&(tris[0]), verts.size(), tris.size()/3);
    vcg_mesh.set_normals( (float*)&(nors[0]) );
    vcg_mesh.update_bb();

    vcg::MyMesh out ;

    vcg::tri::ConvexHull<vcg::MyMesh, vcg::MyMesh>::ComputeConvexHull(vcg_mesh, out) ;

    out.get_vertices(out_verts) ;
    out.get_triangles(out_tris) ;


    // sampler.get_vertices( samples_pos );

}
