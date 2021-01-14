#include "mhx2_importer.hpp"

#include <fstream>
#include <iostream>

using namespace std ;
using namespace Eigen ;
using namespace cvx ;

bool Mhx2Importer::load(const string &fname, const string &mesh)
{
    ifstream strm(fname) ;

    try {
        JSONReader reader(strm) ;

        reader.beginObject() ;

        while ( reader.hasNext() ) {
            string name = reader.nextName() ;
            if ( name == "geometries" ) {
                parseGeometries(reader, mesh) ;
            } else if ( name == "skeleton" ) {
                parseSkeleton(reader) ;
            } else {
                reader.skipValue() ;
            }
        }

        reader.endObject() ;

        return true ;
    }
    catch ( JSONParseException &e ) {
        cerr << "Error parsing MHX2 file: " << fname << " (" ;
        cerr << e.what() << ")" ;
        return false ;
    }


}

static Vector3f toVector3(JSONReader &r) {
    r.beginArray() ;
    Vector3f q (r.nextDouble(), r.nextDouble(), r.nextDouble()) ;
    r.endArray() ;
    return q ;
}

static Vector2f toVector2(JSONReader &r) {
    r.beginArray() ;
    Vector2f q (r.nextDouble(), r.nextDouble()) ;
    r.endArray() ;
    return q ;
}


static Matrix4f toMatrix4(JSONReader &r) {

    Matrix4f m ;
// order?
    r.beginArray() ;
    for( size_t row = 0 ; row < 4 ; row ++ ) {
        r.beginArray() ;
        for( size_t col = 0 ; col < 4 ; col ++ ) {
            m(row, col) = (float)r.nextDouble() ;
        }
       r.endArray() ;
    }
    r.endArray() ;

    return m ;
}

extern float angle_normalized_v3v3(const Vector3f &v1, const Vector3f &v2) ;

bool Mhx2Importer::parseSkeleton(JSONReader &reader) {

    string skeletonName ;
    Vector3f offset ;
    float scale = 1.0 ;

    reader.beginObject() ;

    while ( reader.hasNext() ) {
        string name = reader.nextName() ;
        if ( name == "name" )
            skeletonName = reader.nextString() ;
        else if ( name == "offset" )
            offset = toVector3(reader) ;
        else if ( name == "scale" )
            scale = (float)reader.nextDouble() ;
        else if ( name == "bones" ) {
            reader.beginArray() ;
            while ( reader.hasNext() ) {
                string boneName ;
                Vector3f head, tail ;
                Matrix4f mat ;
                double roll ;
                string parent ;

                reader.beginObject() ;
                while ( reader.hasNext() ) {
                    string name = reader.nextName() ;
                    if ( name == "name" ) {
                        boneName = reader.nextString()  ;
                    } else if ( name == "head" ) {
                        head = toVector3(reader) ;
                    } else if ( name == "tail" ) {
                        tail = toVector3(reader) ;
                    } else if ( name == "matrix" ) {
                        mat = toMatrix4(reader) ;
                    } else if ( name == "roll" ) {
                        roll = reader.nextDouble() ;
                    } else if ( name == "parent" ) {
                        parent = reader.nextString() ;
                    }
                }
                reader.endObject() ;

                MHX2Bone mhbone ;
                mhbone.head_ = head + offset ;
                mhbone.tail_ = tail + offset ;
                mhbone.roll_ = (float)roll ;
                mhbone.bmat_ = mat ;
                mhbone.bmat_.block<3, 1>(0, 3) = mhbone.head_ ;
                mhbone.parent_ = parent ;

                bones_.emplace(boneName, mhbone) ;
            }
            reader.endArray() ;
        } else {
            reader.skipValue() ;
        }
    }

    reader.endObject() ;

    return true ;
}

bool Mhx2Importer::parseGeometries(JSONReader &reader, const string &meshName)
{
    reader.beginArray() ;

    while ( reader.hasNext() ) {
        reader.beginObject() ;

        string geomName ;
        Vector3f offset ;
        float scale = 1.0 ;

        while ( reader.hasNext() ) {
            string name = reader.nextName() ;
            if ( name == "name" )
                geomName = reader.nextString() ;
            else if ( name == "offset" )
                offset = toVector3(reader) ;
            else if ( name == "scale" )
                scale = (float)reader.nextDouble() ;
            else if ( name == "mesh" )
                parseMesh(geomName, reader, offset, scale) ;
            else
                reader.skipValue() ;
        }

        reader.endObject() ;
    }


    reader.endArray() ;

    return true ;
}

bool Mhx2Importer::parseMesh(const string &name, JSONReader &reader, const Vector3f &offset, float scale)
{
    MHX2Mesh &mesh = meshes_[name] ;

    reader.beginObject() ;

    while( reader.hasNext() ) {
        string name = reader.nextName() ;
        if ( name == "vertices" ) {
            reader.beginArray() ;
            while ( reader.hasNext() ) {
                Vector3f v = toVector3(reader) ;
                mesh.vertices_.emplace_back(v) ;
            }
            reader.endArray() ;
        } else if ( name == "uv_coords" ) {
            reader.beginArray() ;
            while ( reader.hasNext() ) {
                Vector2f v = toVector2(reader) ;
                mesh.tex_coords_.emplace_back(v) ;
            }
            reader.endArray() ;
        } else if ( name == "faces" ) {
            reader.beginArray() ;
            while ( reader.hasNext() ) {
                std::vector<uint> indices ;
                reader.beginArray() ;
                while ( reader.hasNext() ) {
                    int idx = reader.nextInt() ;
                    indices.push_back(idx) ;
                }
                reader.endArray() ;
                MHX2Face f(indices) ;
                mesh.faces_.emplace_back(f) ;
            }
            reader.endArray() ;
        } else if ( name == "weights" ) {
            parseVertexGroups(mesh, reader) ;
        } else {
            reader.skipValue();
        }

    }

    reader.endObject() ;

    return true ;
}

bool Mhx2Importer::parseVertexGroups(MHX2Mesh &mesh, JSONReader &reader)
{
    reader.beginObject() ;
    while ( reader.hasNext() ) {
        string gname = reader.nextName() ;

        MHX2VertexGroup &vg = mesh.groups_[gname] ;

        reader.beginArray() ;
        while ( reader.hasNext() ) {
            reader.beginArray() ;
            uint idx = reader.nextInt() ;
            double weight = reader.nextDouble() ;
            vg.idxs_.push_back(idx) ;
            vg.weights_.push_back(weight) ;
            reader.endArray() ;
        }
        reader.endArray() ;
    }
    reader.endObject() ;

    return true ;
}

