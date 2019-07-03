#define USE_DOUBLE
#include "Common/Common.h"
#include "Simulation/TimeManager.h"
#include <Eigen/Dense>
#include "Simulation/SimulationModel.h"
#include "Simulation/TimeStepController.h"
#include <iostream>
#include "Utils/Timing.h"
#include "Simulation/Simulation.h"
#include "Utils/OBJLoader.h"
#include "Simulation/DistanceFieldCollisionDetection.h"

#include <QApplication>
#include <QMainWindow>
#include <cvx/viz/gui/simple_qt_viewer.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/node.hpp>
#include <cvx/viz/scene/material.hpp>
#include <cvx/viz/scene/mesh.hpp>

#include <cvx/util/geometry/point_list.hpp>


using namespace Eigen;
using namespace std;


using namespace cvx::viz ;
using namespace cvx::util ;

/** Create a particle model mesh
*/
INIT_TIMING
INIT_LOGGING

const int nRows = 50;
const int nCols = 50;
const Real width = 10.0;
const Real height = 10.0;
short simulationMethod = 2;
short bendingMethod = 2;

void createMesh()
{
    using namespace PBD ;
    using namespace Utilities ;

    TriangleModel::ParticleMesh::UVs uvs;
    uvs.resize(nRows*nCols);

    const Real dy = width / (Real)(nCols - 1);
    const Real dx = height / (Real)(nRows - 1);

    Vector3r points[nRows*nCols];
    for (int i = 0; i < nRows; i++)
    {
        for (int j = 0; j < nCols; j++)
        {
            const Real y = (Real)dy*j;
            const Real x = (Real)dx*i;
            points[i*nCols + j] = Vector3r(x, 1.0, y);

            uvs[i*nCols + j][0] = x/width;
            uvs[i*nCols + j][1] = y/height;
        }
    }
    const int nIndices = 6 * (nRows - 1)*(nCols - 1);

    TriangleModel::ParticleMesh::UVIndices uvIndices;
    uvIndices.resize(nIndices);

    unsigned int indices[nIndices];
    int index = 0;
    for (int i = 0; i < nRows - 1; i++)
    {
        for (int j = 0; j < nCols - 1; j++)
        {
            int helper = 0;
            if (i % 2 == j % 2)
                helper = 1;

            indices[index] = i*nCols + j;
            indices[index + 1] = i*nCols + j + 1;
            indices[index + 2] = (i + 1)*nCols + j + helper;

            uvIndices[index] = i*nCols + j;
            uvIndices[index + 1] = i*nCols + j + 1;
            uvIndices[index + 2] = (i + 1)*nCols + j + helper;
            index += 3;

            indices[index] = (i + 1)*nCols + j + 1;
            indices[index + 1] = (i + 1)*nCols + j;
            indices[index + 2] = i*nCols + j + 1 - helper;

            uvIndices[index] = (i + 1)*nCols + j + 1;
            uvIndices[index + 1] = (i + 1)*nCols + j;
            uvIndices[index + 2] = i*nCols + j + 1 - helper;
            index += 3;
        }
    }

    SimulationModel *model = Simulation::getCurrent()->getModel();
    model->addTriangleModel(nRows*nCols, nIndices / 3, &points[0], &indices[0], uvIndices, uvs);

    ParticleData &pd = model->getParticles();
    for (unsigned int i = 0; i < pd.getNumberOfParticles(); i++)
    {
        pd.setMass(i, 1.0);
    }

    // Set mass of points to zero => make it static
  //  pd.setMass(0, 0.0);
 //   pd.setMass((nRows-1)*nCols, 0.0);

    // init constraints
    for (unsigned int cm = 0; cm < model->getTriangleModels().size(); cm++)
    {
        if (simulationMethod == 1)
        {
            const unsigned int offset = model->getTriangleModels()[cm]->getIndexOffset();
            const unsigned int nEdges = model->getTriangleModels()[cm]->getParticleMesh().numEdges();
            const IndexedFaceMesh::Edge *edges = model->getTriangleModels()[cm]->getParticleMesh().getEdges().data();
            for (unsigned int i = 0; i < nEdges; i++)
            {
                const unsigned int v1 = edges[i].m_vert[0] + offset;
                const unsigned int v2 = edges[i].m_vert[1] + offset;

                model->addDistanceConstraint(v1, v2);
            }
        }
        else if (simulationMethod == 2)
        {
            const unsigned int offset = model->getTriangleModels()[cm]->getIndexOffset();
            TriangleModel::ParticleMesh &mesh = model->getTriangleModels()[cm]->getParticleMesh();
            const unsigned int *tris = mesh.getFaces().data();
            const unsigned int nFaces = mesh.numFaces();
            for (unsigned int i = 0; i < nFaces; i++)
            {
                const unsigned int v1 = tris[3 * i] + offset;
                const unsigned int v2 = tris[3 * i + 1] + offset;
                const unsigned int v3 = tris[3 * i + 2] + offset;
                model->addFEMTriangleConstraint(v1, v2, v3);
            }
        }
        else if (simulationMethod == 3)
        {
            const unsigned int offset = model->getTriangleModels()[cm]->getIndexOffset();
            TriangleModel::ParticleMesh &mesh = model->getTriangleModels()[cm]->getParticleMesh();
            const unsigned int *tris = mesh.getFaces().data();
            const unsigned int nFaces = mesh.numFaces();
            for (unsigned int i = 0; i < nFaces; i++)
            {
                const unsigned int v1 = tris[3 * i] + offset;
                const unsigned int v2 = tris[3 * i + 1] + offset;
                const unsigned int v3 = tris[3 * i + 2] + offset;
                model->addStrainTriangleConstraint(v1, v2, v3);
            }
        }
        if (bendingMethod != 0)
        {
            const unsigned int offset = model->getTriangleModels()[cm]->getIndexOffset();
            TriangleModel::ParticleMesh &mesh = model->getTriangleModels()[cm]->getParticleMesh();
            unsigned int nEdges = mesh.numEdges();
            const TriangleModel::ParticleMesh::Edge *edges = mesh.getEdges().data();
            const unsigned int *tris = mesh.getFaces().data();
            for (unsigned int i = 0; i < nEdges; i++)
            {
                const int tri1 = edges[i].m_face[0];
                const int tri2 = edges[i].m_face[1];
                if ((tri1 != 0xffffffff) && (tri2 != 0xffffffff))
                {
                    // Find the triangle points which do not lie on the axis
                    const int axisPoint1 = edges[i].m_vert[0];
                    const int axisPoint2 = edges[i].m_vert[1];
                    int point1 = -1;
                    int point2 = -1;
                    for (int j = 0; j < 3; j++)
                    {
                        if ((tris[3 * tri1 + j] != axisPoint1) && (tris[3 * tri1 + j] != axisPoint2))
                        {
                            point1 = tris[3 * tri1 + j];
                            break;
                        }
                    }
                    for (int j = 0; j < 3; j++)
                    {
                        if ((tris[3 * tri2 + j] != axisPoint1) && (tris[3 * tri2 + j] != axisPoint2))
                        {
                            point2 = tris[3 * tri2 + j];
                            break;
                        }
                    }
                    if ((point1 != -1) && (point2 != -1))
                    {
                        const unsigned int vertex1 = point1 + offset;
                        const unsigned int vertex2 = point2 + offset;
                        const unsigned int vertex3 = edges[i].m_vert[0] + offset;
                        const unsigned int vertex4 = edges[i].m_vert[1] + offset;
                        if (bendingMethod == 1)
                            model->addDihedralConstraint(vertex1, vertex2, vertex3, vertex4);
                        else if (bendingMethod == 2)
                            model->addIsometricBendingConstraint(vertex1, vertex2, vertex3, vertex4);
                    }
                }
            }
        }
    }


}


ScenePtr g_scene ;

void updateScene(const PBD::ParticleData &particles, int offset, const PBD::TriangleModel::ParticleMesh &pmesh) {
    NodePtr node = g_scene->findNodeByName("cloth") ;

    auto dr = node->getDrawable(0) ;

    PointList3f vtx, normals ;

    MeshPtr mesh = dynamic_pointer_cast<Mesh>(dr->geometry()) ;

    for( uint i=0 ; i<pmesh.numVertices() ; i++ ) {
        Vector3d pp = particles.getPosition(offset + i) ;
        mesh->vertices().data().at(i) = pp.cast<float>() ;
        mesh->normals().data().at(i) = pmesh.getVertexNormals().at(i).cast<float>();
    }


}

void loadObj(const std::string &filename, PBD::VertexData &vd, Utilities::IndexedFaceMesh &mesh, const Vector3r &scale)
{
    using namespace PBD ;
    using namespace Utilities ;

    std::vector<OBJLoader::Vec3f> x;
    std::vector<OBJLoader::Vec3f> normals;
    std::vector<OBJLoader::Vec2f> texCoords;
    std::vector<MeshFaceIndices> faces;
    OBJLoader::Vec3f s = { (float)scale[0], (float)scale[1], (float)scale[2] };
    OBJLoader::loadObj(filename, &x, &faces, &normals, &texCoords, s);

    mesh.release();
    const unsigned int nPoints = (unsigned int)x.size();
    const unsigned int nFaces = (unsigned int)faces.size();
    const unsigned int nTexCoords = (unsigned int)texCoords.size();
    mesh.initMesh(nPoints, nFaces * 2, nFaces);
    vd.reserve(nPoints);
    for (unsigned int i = 0; i < nPoints; i++)
    {
        vd.addVertex(Vector3r(x[i][0], x[i][1], x[i][2]));
    }
    for (unsigned int i = 0; i < nTexCoords; i++)
    {
        mesh.addUV(texCoords[i][0], texCoords[i][1]);
    }
    for (unsigned int i = 0; i < nFaces; i++)
    {
        // Reduce the indices by one
        int posIndices[3];
        int texIndices[3];
        for (int j = 0; j < 3; j++)
        {
            posIndices[j] = faces[i].posIndices[j] - 1;
            if (nTexCoords > 0)
            {
                texIndices[j] = faces[i].texIndices[j] - 1;
                mesh.addUVIndex(texIndices[j]);
            }
        }

        mesh.addFace(&posIndices[0]);
    }
    mesh.buildNeighbors();

    mesh.updateNormals(vd, 0);
    mesh.updateVertexNormals(vd);

    LOG_INFO << "Number of triangles: " << nFaces;
    LOG_INFO << "Number of vertices: " << nPoints;
}

Vector3r computeInertiaTensorBox(const Real mass, const Real width, const Real height, const Real depth)
{
    const Real Ix = (mass / static_cast<Real>(12.0)) * (height*height + depth*depth);
    const Real Iy = (mass / static_cast<Real>(12.0)) * (width*width + depth*depth);
    const Real Iz = (mass / static_cast<Real>(12.0)) * (width*width + height*height);
    return Vector3r(Ix, Iy, Iz);
}

PBD::DistanceFieldCollisionDetection cd;

NodePtr makeBox(const string &name, const Vector3f &hs, const Matrix4f &tr, const Vector4f &clr) {

    NodePtr box_node(new Node) ;
    box_node->setName(name) ;

    GeometryPtr geom(new BoxGeometry(hs)) ;

    MaterialInstancePtr material(new ConstantMaterialInstance(clr)) ;

    DrawablePtr dr(new Drawable(geom, material)) ;

    box_node->addDrawable(dr) ;

    box_node->matrix() = tr ;

    return box_node ;
}

void createWorld() {
    using namespace PBD ;
    using namespace Utilities ;

   SimulationModel *model = Simulation::getCurrent()->getModel();
   SimulationModel::RigidBodyVector &rb = model->getRigidBodies();

   string fileName = "/home/malasiot/Downloads/cube.obj";

   IndexedFaceMesh mesh;
   VertexData vd;
   loadObj(fileName, vd, mesh, Vector3r(1, 1, 1));

    rb.resize(1);


    rb[0] = new RigidBody() ;
    rb[0]->initBody(0.0,
            Vector3r(3.5, -3.5, 3.5),
            computeInertiaTensorBox(1.0, 0.2, 0.2, 0.2),
            Quaternionr(1.0, 0.0, 0.0, 0.0),
            vd, mesh);

    rb[0]->setMass(0.0);



    Simulation::getCurrent()->getTimeStep()->setCollisionDetection(*model, &cd);
    cd.setTolerance(static_cast<Real>(0.005));

    const std::vector<Vector3r> *vertices1 = rb[0]->getGeometry().getVertexDataLocal().getVertices();
    const unsigned int nVert1 = static_cast<unsigned int>(vertices1->size());
    cd.addCollisionBox(0, CollisionDetection::CollisionObject::RigidBodyCollisionObjectType, &(*vertices1)[0], nVert1, Vector3r(1, 1, 1));

    SimulationModel::TriangleModelVector &tm = model->getTriangleModels();
    ParticleData &pd = model->getParticles();
    for (unsigned int i = 0; i < tm.size(); i++)
    {
        const unsigned int nVert = tm[i]->getParticleMesh().numVertices();
        unsigned int offset = tm[i]->getIndexOffset();
        tm[i]->setFrictionCoeff(static_cast<Real>(0.1));
        cd.addCollisionObjectWithoutGeometry(i, CollisionDetection::CollisionObject::TriangleModelCollisionObjectType, &pd.getPosition(offset), nVert, true);
    }


}
const int NUM_STEPS_PER_RENDER = 1 ;

void timeStep ()
{
    using namespace PBD ;
    using namespace Utilities ;

    // Simulation code
    SimulationModel *model = Simulation::getCurrent()->getModel();
    const unsigned int numSteps = NUM_STEPS_PER_RENDER;
    for (unsigned int i = 0; i < numSteps; i++)
    {

        Simulation::getCurrent()->getTimeStep()->step(*model);
    }

    for (unsigned int i = 0; i < model->getTriangleModels().size(); i++) {
        model->getTriangleModels()[i]->updateMeshNormals(model->getParticles());

        updateScene(model->getParticles(), model->getTriangleModels()[0]->getIndexOffset(), model->getTriangleModels()[0]->getParticleMesh()) ;
    }
}


ScenePtr createScene(PBD::SimulationModel *model) {

    ScenePtr scene(new Scene) ;

    NodePtr node(new Node) ;

    node->setName("cloth") ;
    MaterialInstancePtr material(new PhongMaterialInstance()) ;

    DrawablePtr dr(new Drawable(nullptr, material)) ;

    node->addDrawable(dr) ;

    MeshPtr mesh(new Mesh(Mesh::Triangles)) ;

    const PBD::TriangleModel::ParticleMesh &pmesh = model->getTriangleModels()[0]->getParticleMesh() ;
    const PBD::ParticleData &particles = model->getParticles() ;
    uint offset = model->getTriangleModels()[0]->getIndexOffset() ;
    for( uint i=0 ; i<pmesh.numVertices() ; i++ ) {
        Vector3d pp = particles.getPosition(offset + i) ;
        mesh->vertices().data().push_back(pp.cast<float>()) ;
        mesh->normals().data().push_back(pmesh.getVertexNormals().at(i).cast<float>());
    }

    mesh->vertices().setDynamicData(true) ;
    mesh->normals().setDynamicData(true) ;

    for(uint i=0 ; i<pmesh.numFaces() * 3; i++ ) {
         mesh->vertices().indices().push_back(pmesh.getFaces().at(i)) ;
    }

    dr->setGeometry(mesh) ;

    scene->addChild(node) ;

    Affine3f tr ;
    tr.setIdentity() ;
    tr.translate(Vector3f(3.5, -3.5, 3.5)) ;

    scene->addChild(makeBox("box", {0.5, 0.5, 0.5},tr.matrix(), {1, 0, 0, 1} )) ;


    // create new scene and add light

    std::shared_ptr<DirectionalLight> dl( new DirectionalLight(Vector3f(0.5, 0.5, 1)) ) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(dl) ;

    return scene ;

}


int main(int argc, char *argv[]) {

    using namespace PBD ;
    using namespace Utilities ;

    SimulationModel *model = new SimulationModel();
    model->init();
    Simulation::getCurrent()->setModel(model);

    TimeManager::getCurrent()->setTimeStepSize(static_cast<Real>(0.005)) ;
    createMesh() ;
    createWorld() ;

    QApplication app(argc, argv);

    SimpleQtViewer *viewer = new SimpleQtViewer();

    g_scene = createScene(model) ;
    viewer->setScene(g_scene) ;
    viewer->initCamera({0, 0, 0}, 10.0f) ;

    QMainWindow window ;
    window.setCentralWidget(viewer) ;
    window.resize(512, 512) ;
    window.show() ;

    viewer->setAnimationCallback([&](float ) {
        timeStep() ;
    }) ;

    viewer->startAnimations() ;

    return app.exec();



}
