
#include "draco_decoder.h"
#include "rudp_client.h"

static bool output = true;

int main(int argc, char **argv)
{
  draco_decoder decoder;
  decoder.OnDecompressDone = [&](mesh_object *data)
  {
    if(output)
    {
      output = false;
      std::cout << "vertices " << std::endl;
      for(int i=0;i<std::min(20, static_cast<int>(data->vertices.size()));i++)
      {
        std::cout << " " << data->vertices[i];
      }
      std::cout << std::endl << std::endl;
    
      std::cout << "normals " << std::endl;
      for(int i=0;i<std::min(20, static_cast<int>(data->normals.size()));i++)
      {
        std::cout << " " << data->normals[i];
      }
      std::cout << std::endl << std::endl;

      std::cout << "faces " << std::endl;
      for(int i=0;i<std::min(20, static_cast<int>(data->faces.size()));i++)
      {
        std::cout << " " << data->faces[i];
      }
      std::cout << std::endl << std::endl;

      std::cout << "uv " << std::endl;
      for(int i=0;i<std::min(20, static_cast<int>(data->uv.size()));i++)
      {
        std::cout << " " << data->uv[i];
      }
        std::cout << std::endl << std::endl;
    }
  };

  rudp_client receiver = rudp_client();

  receiver.OnReceive = [&](const char *data, size_t dataLength) {
    printf("Received: %i bytes\n", (unsigned int)dataLength);

    // if(output)
    // {
    //   output = false;
    //   decoder.DecompressIntoFile(data, dataLength, "model.ply");
    // }
    // return;

    decoder.Decompress(data, dataLength);
 
    // ================= END Substitute by buffer sink
  };

  while(!receiver.Connect("131.159.10.99", 9898));

  // Keep application alive
  std::getchar();

  receiver.CleanUp();
  return 0;
}
