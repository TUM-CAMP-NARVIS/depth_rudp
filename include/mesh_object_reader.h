#ifndef MESH_OBJECT_READER_H_
#define MESH_OBJECT_READER_H_

#include <vector>

#include "draco/core/decoder_buffer.h"
#include "draco/core/draco_types.h"
#include "draco/core/status.h"
#include "draco/core/status_or.h"
#include "draco/io/mesh_io.h"
#include "draco/io/point_cloud_io.h"
#include "draco/mesh/mesh.h"
#include "mesh_object.h"

class mesh_object_reader
{
    public:
    static bool ReadMesh(mesh_object *inMesh, draco::Mesh *out_mesh);
private:

};


#endif