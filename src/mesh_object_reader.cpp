#include "mesh_object_reader.h"
#include <iterator>
using namespace draco;

bool mesh_object_reader::ReadMesh(mesh_object *inMesh, Mesh *out_mesh)
{
  auto pc = static_cast<draco::PointCloud *>(out_mesh);

  if(inMesh->vertices.size() % 3 != 0 || inMesh->normals.size() % 3 != 0 || inMesh->faces.size() % 3 != 0 || inMesh->uv.size() % 2 != 0)
  {
      // TODO: error message vertex array size is not a multiple of 3
      return false;
  }
  size_t numVertices = inMesh->vertices.size() / 3;
  size_t numFaces = inMesh->faces.size() / 3;
  size_t numNormals = inMesh->normals.size() / 3;
  size_t numUV = inMesh->uv.size() / 2;

  // TODO check normals and uv either equals zero or numVertices

  if(out_mesh)
  {
    out_mesh->SetNumFaces(numFaces);

    Mesh::Face face;
    FaceIndex face_index(0);
    std::vector<int>::iterator it = inMesh->faces.begin();
    while(it < inMesh->faces.end())
    {
        face[0] = *it;
        it++;
        face[1] = *it;
        it++;
        face[2] = *it;
        it++;

        out_mesh->SetFace(face_index, face);
        face_index++;
    }
    out_mesh->SetNumFaces(face_index.value());
  }

  { // Read vertices
    pc->set_num_points(numVertices);

    GeometryAttribute va;
    va.Init(GeometryAttribute::POSITION, nullptr, 3, DataType::DT_FLOAT32, false, DataTypeLength(DT_FLOAT32) * 3, 0);
    const int att_id = pc->AddAttribute(va, true, numVertices);

    memcpy(pc->attribute(att_id)->buffer()->data(), inMesh->vertices.data(), numVertices*sizeof(float) * 3);

  }

  if(numNormals > 0)
  { // Read normals
    GeometryAttribute na;
    na.Init(GeometryAttribute::NORMAL, nullptr, 3, DataType::DT_FLOAT32, false, DataTypeLength(DT_FLOAT32) * 3, 0);
    const int att_id = pc->AddAttribute(na, true, numNormals);

    memcpy(pc->attribute(att_id)->buffer()->data(), inMesh->normals.data(), numNormals*sizeof(float) * 3);
  }

  if(numUV > 0)
  { // Read texture coordinates
    GeometryAttribute na;
    na.Init(GeometryAttribute::TEX_COORD, nullptr, 2, DataType::DT_FLOAT32, false, DataTypeLength(DT_FLOAT32) * 2, 0);
    const int att_id = pc->AddAttribute(na, true, numUV);

    memcpy(pc->attribute(att_id)->buffer()->data(), inMesh->uv.data(), numUV*sizeof(float) * 2);
  }

  // In case there are no faces this is just a point cloud which does
  // not require deduplication.
  if (out_mesh && out_mesh->num_faces() != 0) 
  {
 #ifdef DRACO_ATTRIBUTE_VALUES_DEDUPLICATION_SUPPORTED
    if (!pc->DeduplicateAttributeValues())
        return false;
#endif
#ifdef DRACO_ATTRIBUTE_INDICES_DEDUPLICATION_SUPPORTED
    pc->DeduplicatePointIds();
#endif
  }

  return true;
}