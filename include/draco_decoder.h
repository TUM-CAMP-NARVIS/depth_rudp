
#include "draco/compression/decode.h"
#include "draco/core/cycle_timer.h"
#include "draco/io/obj_encoder.h"
#include "draco/io/parser_utils.h"
#include "draco/io/ply_encoder.h"
#include "mesh_object.h"
#include "queued_task.h"
#include <functional>

class draco_decoder
{
public:
    draco_decoder();

    int DecompressIntoFile(const char* inBuffer, size_t inDataLength, std::string fileName);
    int Decompress(const char* inBuffer, size_t inDataLength);

    std::function<void(mesh_object*)> OnDecompressDone;

private:

    int DecompressMesh(draco::DecoderBuffer *buffer, std::unique_ptr<draco::Mesh> &outMesh);
    int DecompressPointCloud(draco::DecoderBuffer *buffer, std::unique_ptr<draco::PointCloud> &outPointCloud);

    bool Mesh2MeshObject(const draco::Mesh &inMesh, mesh_object *out_mesh_object);
    bool PointCloud2MeshObject(const draco::PointCloud &inPointcloud, mesh_object *out_mesh_object);
    
    std::shared_ptr<queued_task<std::pair<char*, size_t>>> bufferTask;

    std::shared_ptr<queued_task<std::shared_ptr<draco::Mesh>>> meshTask;
    std::shared_ptr<queued_task<std::shared_ptr<draco::PointCloud>>> pointcloudTask;

};