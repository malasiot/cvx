#define USE_DOUBLE
#include "Common/Common.h"
#include "Simulation/TimeManager.h"
#include <Eigen/Dense>
#include "Simulation/SimulationModel.h"
#include "Simulation/TimeStepController.h"
#include <iostream>
#include "Utils/Timing.h"
#include "Simulation/Simulation.h"

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

const int nRows = 15;
const int nCols = 15;
const Real width = 10.0;
const Real height = 10.0;
short simulationMethod = 1;
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
    pd.setMass(0, 0.0);
    pd.setMass((nRows-1)*nCols, 0.0);

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

    MeshPtr mesh(new Mesh(Mesh::Triangles)) ;

    for( uint i=0 ; i<pmesh.numVertices() ; i++ ) {
        Vector3d pp = particles.getPosition(offset + i) ;
        mesh->vertices().data().push_back(pp.cast<float>()) ;
        mesh->normals().data().push_back(pmesh.getVertexNormals().at(i).cast<float>());
    }

    for(uint i=0 ; i<pmesh.numFaces() * 3; i++ ) {
        mesh->vertices().indices().push_back(pmesh.getFaces().at(i)) ;
    }

    dr->setGeometry(mesh) ;
}

const int NUM_STEPS_PER_RENDER = 5 ;

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


ScenePtr createScene() {

    ScenePtr scene(new Scene) ;

    NodePtr node(new Node) ;

    node->setName("cloth") ;
    MaterialInstancePtr material(new PhongMaterialInstance()) ;

    DrawablePtr dr(new Drawable(nullptr, material)) ;

    node->addDrawable(dr) ;

    scene->addChild(node) ;


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

    TimeManager::getCurrent()->setTimeStepSize(static_cast<Real>(0.05)) ;
    createMesh() ;

    QApplication app(argc, argv);

    SimpleQtViewer *viewer = new SimpleQtViewer();

    g_scene = createScene() ;
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
