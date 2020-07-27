#ifndef MESH_OBJECT_H_
#define MESH_OBJECT_H_
#include <vector>

struct mesh_object
{
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<int> faces;
    std::vector<float> uv;
};
#endif