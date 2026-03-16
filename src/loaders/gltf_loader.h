#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../core/object_3d.h"

class GLTFLoader {
 public:
  explicit GLTFLoader(const std::string& path);
  ~GLTFLoader();
  void Load();
  std::unique_ptr<Object3D> object_3d_;

 private:
  void ParseGLB();
  void ProcessScene();
  void ProcessNode(int node_index, Object3D* parent);
  std::unique_ptr<class MeshObject> BuildMeshObject(int mesh_index, int primitive_index);
  std::vector<float>        ReadAccessorAsFloats(int accessor_index) const;
  std::vector<unsigned int> ReadAccessorAsUInts(int accessor_index) const;
  unsigned int LoadGLTexture(int image_index, int sampler_index) const;
  unsigned int ResolveTexture(int texture_index);
  std::string path_;
  std::vector<unsigned char> json_bytes_;
  std::vector<unsigned char> bin_buffer_;
  struct Impl;
  std::unique_ptr<Impl> impl_;
  std::vector<unsigned int> gl_texture_cache_;
};
