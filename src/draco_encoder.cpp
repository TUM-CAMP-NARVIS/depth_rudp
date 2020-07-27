
#include "draco_encoder.h"
#include "mesh_object_reader.h"

draco_encoder::draco_encoder(int cl, int qp)
{
    const int speed = 10 - cl;

    encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, qp);

    encoder.SetSpeedOptions(speed, speed);

//==============================================================================================
// Converts mesh_object into a draco mesh

    readMeshTask = std::make_shared<queued_task<mesh_object>>(2);

    readMeshTask->SetTask([&](mesh_object data)
    {
      std::shared_ptr<draco::Mesh> mesh = std::make_shared<draco::Mesh>();

      if(mesh_object_reader::ReadMesh(&data, mesh.get()))
      {
          compressTask->EnqueueData(mesh);
      }
    });
    readMeshTask->StartTaskLoop();

//==============================================================================================
// Compresses and serializes draco mesh into buffer

    compressTask = std::make_shared<queued_task<std::shared_ptr<draco::Mesh>>>(2);

    compressTask->SetTask([&](std::shared_ptr<draco::Mesh> data)
    {
      draco::EncoderBuffer buffer;
      const bool input_is_mesh = data->num_faces() > 0;

      if(data->num_faces() > 0)
      {
          Compress(*data, buffer);
      }else
      {
          draco::PointCloud *pc = data.get();
          Compress(*pc, buffer);
      }
      if(OnCompressDone)
        OnCompressDone(&buffer);
    });
    compressTask->StartTaskLoop();
}

void draco_encoder::Compress(mesh_object *inMeshObject)
{
  readMeshTask->EnqueueData(*inMeshObject);
}

int draco_encoder::Compress(draco::Mesh &inMesh, draco::EncoderBuffer &outBuffer)
{
    const draco::Status status = encoder.EncodeMeshToBuffer(inMesh, &outBuffer);
    if (!status.ok())
    {
        printf("Failed to encode the mesh.\n");
        printf("%s\n", status.error_msg());
        return -1;
    }

    //printf("\nEncoded size = %zu bytes\n\n", outBuffer.size());
    return 0;
}

int draco_encoder::Compress(draco::PointCloud &inPointCloud, draco::EncoderBuffer &outBuffer)
{
  // Encode the geometry.
  const draco::Status status = encoder.EncodePointCloudToBuffer(inPointCloud, &outBuffer);
  if (!status.ok())
  {
    printf("Failed to encode the point cloud.\n");
    printf("%s\n", status.error_msg());
    return -1;
  }

  return 0;
}