
#include "draco_decoder.h"
using namespace draco;

draco_decoder::draco_decoder()
{
    bufferTask = std::make_shared<queued_task<std::pair<char*, size_t>>>(2),
    bufferTask->SetTask([&](std::pair<char*, size_t> data)
    {

        draco::DecoderBuffer buffer;
        buffer.Init(data.first, data.second);

        auto type_statusor = Decoder::GetEncodedGeometryType(&buffer);
        if (!type_statusor.ok())
        {
            return 0;
        }
        const EncodedGeometryType geom_type = type_statusor.value();

        if (geom_type == TRIANGULAR_MESH)
        { 
            std::unique_ptr<Mesh> outMesh;

            if(DecompressMesh(&buffer, outMesh))
            {
                meshTask->EnqueueData(std::move(outMesh));
            }
        }
        else if (geom_type == POINT_CLOUD)
        { 
            std::unique_ptr<PointCloud> outPointCloud;

            if(DecompressPointCloud(&buffer, outPointCloud))
            {
                pointcloudTask->EnqueueData(std::move(outPointCloud));
            }
            
        }
        free(data.first);
    });
    bufferTask->StartTaskLoop();

    meshTask = std::make_shared<queued_task<std::shared_ptr<draco::Mesh>>>(2);
    meshTask->SetTask([&](std::shared_ptr<draco::Mesh> data)
    {

        mesh_object outRaw;
        Mesh2MeshObject(*data, &outRaw);
        if(OnDecompressDone)
            OnDecompressDone(&outRaw);
    });
    meshTask->StartTaskLoop();

    pointcloudTask = std::make_shared<queued_task<std::shared_ptr<draco::PointCloud>>>(2);
    pointcloudTask->SetTask([&](std::shared_ptr<draco::PointCloud> data)
    {
        mesh_object outRaw;
        PointCloud2MeshObject(*data, &outRaw);
        if(OnDecompressDone)
            OnDecompressDone(&outRaw);
    });
    pointcloudTask->StartTaskLoop();
}


int draco_decoder::DecompressIntoFile(const char *inBuffer, size_t inDataLength, std::string fileName)
{
    PlyEncoder ply_encoder;

    DecoderBuffer buffer;
    buffer.Init(inBuffer, inDataLength);

    auto type_statusor = Decoder::GetEncodedGeometryType(&buffer);
    if (!type_statusor.ok())
    {
        return 0;
    }
    const EncodedGeometryType geom_type = type_statusor.value();

    if (geom_type == TRIANGULAR_MESH)
    {
        std::unique_ptr<Mesh> outMesh;

        if(DecompressMesh(&buffer, outMesh))
        {
            if (!ply_encoder.EncodeToFile(*outMesh, fileName))
            {
                printf("Failed to store the decoded mesh as PLY.\n");
                return -1;
            }
        }
    }
    else if (geom_type == POINT_CLOUD)
    {
        std::unique_ptr<PointCloud> outPointCloud;

        if(DecompressPointCloud(&buffer, outPointCloud))
        {
            if (!ply_encoder.EncodeToFile(*outPointCloud, fileName))
            {
                printf("Failed to store the decoded mesh as PLY.\n");
                return -1;
            }
        }
        
    }

    return 0;
}

int draco_decoder::Decompress(const char* inBuffer, size_t inDataLength)
{
    char* byteBuffer = (char*) malloc(inDataLength);
    memcpy(byteBuffer, inBuffer, inDataLength);

    bufferTask->EnqueueData(std::pair<char*, size_t>(byteBuffer, inDataLength));

    return 0;
}


int draco_decoder::DecompressMesh(DecoderBuffer *buffer, std::unique_ptr<Mesh> &outMesh)
{
    Decoder decoder;
    auto statusor = decoder.DecodeMeshFromBuffer(buffer);
    if (!statusor.ok())
    {
        return 0;
    }
    outMesh = std::move(statusor).value();

    return 1;
}

int draco_decoder::DecompressPointCloud(DecoderBuffer *buffer, std::unique_ptr<PointCloud> &outPointCloud)
{
    Decoder decoder;
    auto statusor = decoder.DecodePointCloudFromBuffer(buffer);
    if (!statusor.ok())
    {
        return 0;
    }
    outPointCloud = std::move(statusor).value();

    return 1;
}

bool draco_decoder::Mesh2MeshObject(const Mesh &inMesh, mesh_object *out_mesh_object)
{   
    const PointCloud &in_point_cloud = static_cast<const PointCloud &>(inMesh);

    // Read positions
    const PointAttribute *const pos_att = in_point_cloud.GetNamedAttribute(GeometryAttribute::POSITION);
    if (pos_att == nullptr || pos_att->size() == 0)
    {
        return false;
    }
    else
    {
        int size = pos_att->size() * 3;
        float *buffer = (float*)malloc(size*sizeof(float));
        
        out_mesh_object->vertices.resize(size);
        for (AttributeValueIndex i(0); i < static_cast<uint32_t>(pos_att->size()); ++i) 
        {
            pos_att->ConvertValue<float>(i, 3, &buffer[i.value()*3]);
        }
        memcpy(out_mesh_object->vertices.data(), buffer, size*sizeof(float));
        free(buffer);
    }

    // Read faces
    out_mesh_object->faces.clear();
    for (FaceIndex i(0); i < inMesh.num_faces(); ++i) {

        for (int j = 0; j < 3; ++j) {
            out_mesh_object->faces.push_back(inMesh.face(i)[j].value());
        }

    }

    // Read normals
    const PointAttribute *const normal_att = in_point_cloud.GetNamedAttribute(GeometryAttribute::NORMAL);
    if (normal_att != nullptr && normal_att->size() > 0)
    {
        int size = normal_att->size() * 3;
        float *buffer = (float*)malloc(size*sizeof(float));
        
        out_mesh_object->normals.resize(size);
        for (AttributeValueIndex i(0); i < static_cast<uint32_t>(normal_att->size()); ++i) 
        {
            normal_att->ConvertValue<float>(i, 3, &buffer[i.value()*3]);
        }
        memcpy(out_mesh_object->normals.data(), buffer, size*sizeof(float));
        free(buffer);
    }

        // Read tex coords
    const PointAttribute *const tex_coord_att = in_point_cloud.GetNamedAttribute(GeometryAttribute::TEX_COORD);
    if (tex_coord_att != nullptr && tex_coord_att->size() > 0)
    {
        int size = tex_coord_att->size() * 2;
        float *buffer = (float*)malloc(size*sizeof(float));
        
        out_mesh_object->uv.resize(size);
        for (AttributeValueIndex i(0); i < static_cast<uint32_t>(tex_coord_att->size()); ++i) 
        {
            tex_coord_att->ConvertValue<float>(i, 2, &buffer[i.value()*2]);
        }
        memcpy(out_mesh_object->uv.data(), buffer, size*sizeof(float));
        free(buffer);
    }

    return true;
}

bool draco_decoder::PointCloud2MeshObject(const PointCloud &in_point_cloud, mesh_object *out_mesh_object)
{
    // Read positions
    const PointAttribute *const pos_att = in_point_cloud.GetNamedAttribute(GeometryAttribute::POSITION);
    if (pos_att == nullptr || pos_att->size() == 0)
        return false;

    int posSize = pos_att->size() * 3;
    float *buffer = (float*)malloc(posSize*sizeof(float));
    
    out_mesh_object->vertices.resize(posSize);
    for (AttributeValueIndex i(0); i < static_cast<uint32_t>(pos_att->size()); ++i) 
    {
        pos_att->ConvertValue<float>(i, 3, &buffer[i.value()*3]);
    }
    memcpy(out_mesh_object->vertices.data(), buffer, posSize*sizeof(float));
    free(buffer);
    return true;
}