#pragma once
#include "VertexArrayObject.h"
#include "data/ModelData.h"
#include "graphics/Shader.h"
#include <GLFW/glfw3.h>
namespace engine {
class Mesh {
public:
  Mesh(const data::MeshData &meshData, const data::MaterialData &material);

  void Draw(Shader &shader);
  void DrawInstanced(Shader &shader,
                     const std::vector<glm::mat4> &instanceMatrices);
  void DrawGeometryOnly(Shader &shader);

private:
  void SetupMesh(const data::MeshData &meshData);
  void SetupMaterial(Shader &shader, const data::MaterialData &material);
  std::shared_ptr<VertexArrayObject> m_vao;
  std::shared_ptr<VertexBuffer> m_vbo;
  std::shared_ptr<IndexBuffer> m_ebo;
  std::shared_ptr<VertexBuffer> m_instanceBuffer;
  size_t m_indexCount;
  data::MaterialData m_material;
};

class Model {
public:
  Model(const std::string &path) {
    m_modelData = data::AssimpLoader::LoadModel(path);
    SetupModel();
  }

  void Draw(Shader &shader);
  void DrawGeometryOnly(Shader &shader);
  void DrawInstanced(Shader &shader,
                     const std::vector<glm::mat4> &instanceMatrices);
  size_t GetMeshCount() const { return m_meshes.size(); }
  size_t GetTriangleCount() const;

private:
  void SetupModel();

  data::ModelData m_modelData;
  std::vector<Mesh> m_meshes;
  std::vector<data::TextureData> m_texturesLoaded; // texture cache
};
} // namespace engine