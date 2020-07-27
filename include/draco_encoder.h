
#include "draco/compression/encode.h"
#include "draco/io/mesh_io.h"
#include "draco/io/point_cloud_io.h"
#include "mesh_object.h"
#include "queued_task.h"
#include <mutex>
#include <functional>

class draco_encoder
{
public:
    draco_encoder(int cl = 1, int qp = 14);

    void Compress(mesh_object *inMeshObject);

    bool UpdateAvailable();

    draco::EncoderBuffer GetLatestBuffer();

    std::function<void(draco::EncoderBuffer *data)> OnCompressDone;

private:
    int Compress(draco::Mesh &inMesh, draco::EncoderBuffer &outBuffer);

    int Compress(draco::PointCloud &inPointCloud, draco::EncoderBuffer &outBuffer);

    draco::Encoder encoder;
    std::shared_ptr<queued_task<mesh_object>> readMeshTask;

    std::shared_ptr<queued_task<std::shared_ptr<draco::Mesh>>> compressTask;
};