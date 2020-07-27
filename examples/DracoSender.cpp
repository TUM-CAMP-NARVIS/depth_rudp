
#include "draco/io/mesh_io.h"
#include "draco/io/point_cloud_io.h"
#include "mesh_object_reader.h"
#include "mesh_object.h"
#include "rudp_server.h"

int main(int argc, char **argv)
{

std::vector<float> vertices
{
  0, 0, 0,
	1, 0, 0,
	1, 1, 0,
	0, 1, 0,
	0, 1, 1,
	1, 1, 1,
	1, 0, 1,
	0, 0, 1
};

std::vector<float> normals
{
  1, 0, 0,
  1, 0, 0,
  1, 0, 0,
  1, 0, 0,
  1, 0, 0,
  1, 0, 0,
  1, 0, 0,
  1, 0, 0,
};

std::vector<int> faces
{
  0, 2, 1, //face front
	0, 3, 2,
	2, 3, 4, //face top
	2, 4, 5,
	1, 2, 5, //face right
	1, 5, 6,
	0, 7, 4, //face left
	0, 4, 3,
	5, 4, 7, //face back
	5, 7, 6,
	0, 6, 7, //face bottom
	0, 1, 6
};

std::vector<float> uv
{
  0, 1,
  0, 1,
  0, 1,
  0, 1,
  0, 1,
  0, 1,
  0, 1,
  0, 1
};


vertices.clear();
int gridMax = 200;
for(int x=0;x<gridMax;x++)
{
  for(int y=0;y<gridMax;y++)
  {
    for(int z=0;z<gridMax;z++)
    {
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
    }
  }
}



  // ============= Mesh Sender
  std::cout<<"server start" << std::endl;

  rudp_server sender = rudp_server();

  std::cout<<"sender init" << std::endl;

  sender.Init("131.159.10.99", 9898);

  float updateRateMS = 1.0 / 15;

  while (true)
  {
    mesh_object mesh = mesh_object();
    mesh.vertices = vertices;
    //mesh.normals = normals;
    //mesh.faces = faces;
    //mesh.uv = uv;
  
    sender.Send(&mesh);

    //usleep((int)(updateRateMS * 1000 * 1000)); // Limit Bitrate
  }

  sender.CleanUp();
  // ============= END Mesh Sender

  return 1;
}
