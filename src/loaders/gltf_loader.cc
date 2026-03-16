#include "gltf_loader.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>

#include <glad/glad.h>
#include <glog/logging.h>
#include <glm/glm.hpp>
#include <stab/stab_image.h>

#include "../../deps/nlohmann/json.hpp"

#include "../core/geometry.h"
#include "../core/material.h"
#include "../core/mesh_object.h"
#include "../core/object_3d.h"

using json = nlohmann::json;

static constexpr uint32_t kGLBMagic  = 0x46546C67u;
static constexpr uint32_t kChunkJSON = 0x4E4F534Au;
static constexpr uint32_t kChunkBIN  = 0x004E4942u;

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------
struct GLTFLoader::Impl {
  json doc;
};

// ---------------------------------------------------------------------------
// Small file-scope helpers
// ---------------------------------------------------------------------------
static int ComponentTypeSize(int ct) {
  switch (ct) {
    case 5120: return 1;
    case 5121: return 1;
    case 5122: return 2;
    case 5123: return 2;
    case 5125: return 4;
    case 5126: return 4;
    default:   return 0;
  }
}

static int TypeNumComponents(const std::string& t) {
  if (t == "SCALAR") return 1;
  if (t == "VEC2")   return 2;
  if (t == "VEC3")   return 3;
  if (t == "VEC4")   return 4;
  if (t == "MAT2")   return 4;
  if (t == "MAT3")   return 9;
  if (t == "MAT4")   return 16;
  return 0;
}

static GLint GltfWrapToGL(int w) {
  switch (w) {
    case 33071: return GL_CLAMP_TO_EDGE;
    case 33648: return GL_MIRRORED_REPEAT;
    case 10497: return GL_REPEAT;
    default:    return GL_REPEAT;
  }
}

static GLint GltfFilterToGL(int f, bool is_min) {
  switch (f) {
    case 9728: return GL_NEAREST;
    case 9729: return GL_LINEAR;
    case 9984: return GL_NEAREST_MIPMAP_NEAREST;
    case 9985: return GL_LINEAR_MIPMAP_NEAREST;
    case 9986: return GL_NEAREST_MIPMAP_LINEAR;
    case 9987: return GL_LINEAR_MIPMAP_LINEAR;
    default:   return is_min ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
  }
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
GLTFLoader::GLTFLoader(const std::string& path)
    : path_(path), impl_(std::make_unique<Impl>()) {}

GLTFLoader::~GLTFLoader() = default;

// ---------------------------------------------------------------------------
// Load (entry point)
// ---------------------------------------------------------------------------
void GLTFLoader::Load() {
  object_3d_ = std::make_unique<Object3D>("gltf_root");
  ParseGLB();

  const json& doc = impl_->doc;
  if (doc.is_null()) {
    LOG(ERROR) << "GLTFLoader: JSON is null after parsing " << path_;
    return;
  }

  int num_images = doc.contains("images")
                       ? static_cast<int>(doc["images"].size())
                       : 0;
  gl_texture_cache_.assign(num_images, 0u);

  ProcessScene();
}

// ---------------------------------------------------------------------------
// GLB parsing
// ---------------------------------------------------------------------------
void GLTFLoader::ParseGLB() {
  std::ifstream file(path_, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    LOG(ERROR) << "GLTFLoader: cannot open " << path_;
    return;
  }
  std::streamsize file_size = file.tellg();
  file.seekg(0);
  std::vector<unsigned char> data(static_cast<size_t>(file_size));
  if (!file.read(reinterpret_cast<char*>(data.data()), file_size)) {
    LOG(ERROR) << "GLTFLoader: read failed " << path_;
    return;
  }
  if (file_size < 12) {
    LOG(ERROR) << "GLTFLoader: file too small " << path_;
    return;
  }

  uint32_t magic, version, total_length;
  std::memcpy(&magic,        data.data(),     4);
  std::memcpy(&version,      data.data() + 4, 4);
  std::memcpy(&total_length, data.data() + 8, 4);

  if (magic != kGLBMagic) {
    LOG(ERROR) << "GLTFLoader: bad GLB magic in " << path_;
    return;
  }
  if (version != 2) {
    LOG(ERROR) << "GLTFLoader: unsupported GLB version " << version;
    return;
  }

  size_t offset = 12;
  while (offset + 8 <= static_cast<size_t>(file_size)) {
    uint32_t chunk_length, chunk_type;
    std::memcpy(&chunk_length, data.data() + offset,     4);
    std::memcpy(&chunk_type,   data.data() + offset + 4, 4);
    offset += 8;

    if (offset + chunk_length > static_cast<size_t>(file_size)) {
      LOG(ERROR) << "GLTFLoader: chunk overflows file " << path_;
      break;
    }

    if (chunk_type == kChunkJSON) {
      json_bytes_.assign(data.data() + offset,
                         data.data() + offset + chunk_length);
      try {
        impl_->doc = json::parse(json_bytes_.begin(), json_bytes_.end());
      } catch (const json::parse_error& e) {
        LOG(ERROR) << "GLTFLoader: JSON parse error: " << e.what();
      }
    } else if (chunk_type == kChunkBIN) {
      bin_buffer_.assign(data.data() + offset,
                         data.data() + offset + chunk_length);
    }

    offset += chunk_length;
    if (chunk_length % 4 != 0) offset += 4 - (chunk_length % 4);
  }
}

// ---------------------------------------------------------------------------
// Scene / node traversal
// ---------------------------------------------------------------------------
void GLTFLoader::ProcessScene() {
  const json& doc = impl_->doc;
  int scene_index = doc.value("scene", 0);

  if (!doc.contains("scenes") ||
      scene_index >= static_cast<int>(doc["scenes"].size())) {
    LOG(ERROR) << "GLTFLoader: no valid scene in " << path_;
    return;
  }

  const json& scene = doc["scenes"][scene_index];
  if (!scene.contains("nodes")) return;

  for (const auto& n : scene["nodes"])
    ProcessNode(n.get<int>(), object_3d_.get());
}

void GLTFLoader::ProcessNode(int node_index, Object3D* parent) {
  const json& doc = impl_->doc;

  if (!doc.contains("nodes") || node_index < 0 ||
      node_index >= static_cast<int>(doc["nodes"].size())) {
    LOG(ERROR) << "GLTFLoader: invalid node index " << node_index;
    return;
  }
  const json& node = doc["nodes"][node_index];

  if (node.contains("mesh")) {
    int mesh_index = node["mesh"].get<int>();
    if (doc.contains("meshes") &&
        mesh_index < static_cast<int>(doc["meshes"].size())) {
      const json& mesh = doc["meshes"][mesh_index];
      if (mesh.contains("primitives")) {
        int np = static_cast<int>(mesh["primitives"].size());
        for (int pi = 0; pi < np; ++pi) {
          auto obj = BuildMeshObject(mesh_index, pi);
          if (obj) parent->Add(std::move(obj));
        }
      }
    } else {
      LOG(ERROR) << "GLTFLoader: invalid mesh index " << mesh_index;
    }
  }

  if (node.contains("children"))
    for (const auto& c : node["children"])
      ProcessNode(c.get<int>(), parent);
}

// ---------------------------------------------------------------------------
// Mesh / material building
// ---------------------------------------------------------------------------
std::unique_ptr<MeshObject> GLTFLoader::BuildMeshObject(int mesh_index,
                                                        int primitive_index) {
  const json& doc       = impl_->doc;
  const json& primitive = doc["meshes"][mesh_index]["primitives"][primitive_index];
  const json  empty     = json::object();
  const json& attrs     = primitive.contains("attributes")
                              ? primitive["attributes"] : empty;

  Geometry geo;

  // POSITION (required)
  if (!attrs.contains("POSITION")) {
    LOG(ERROR) << "GLTFLoader: primitive missing POSITION";
    return nullptr;
  }
  {
    auto pos = ReadAccessorAsFloats(attrs["POSITION"].get<int>());
    size_t n = pos.size() / 3;
    geo.vertices.resize(n);
    for (size_t i = 0; i < n; ++i)
      geo.vertices[i].position = {pos[i*3], pos[i*3+1], pos[i*3+2]};
  }

  // NORMAL
  if (attrs.contains("NORMAL")) {
    auto nrm = ReadAccessorAsFloats(attrs["NORMAL"].get<int>());
    size_t n = std::min(geo.vertices.size(), nrm.size() / 3);
    for (size_t i = 0; i < n; ++i)
      geo.vertices[i].normal = {nrm[i*3], nrm[i*3+1], nrm[i*3+2]};
  }

  // TEXCOORD_0 (float or normalised uint8/uint16)
  if (attrs.contains("TEXCOORD_0")) {
    auto uv = ReadAccessorAsFloats(attrs["TEXCOORD_0"].get<int>());
    size_t n = std::min(geo.vertices.size(), uv.size() / 2);
    for (size_t i = 0; i < n; ++i)
      geo.vertices[i].uv = {uv[i*2], uv[i*2+1]};
  }

  // TANGENT (VEC4; w = bitangent sign, ignored for now)
  if (attrs.contains("TANGENT")) {
    auto tan = ReadAccessorAsFloats(attrs["TANGENT"].get<int>());
    size_t n = std::min(geo.vertices.size(), tan.size() / 4);
    for (size_t i = 0; i < n; ++i)
      geo.vertices[i].tangent = {tan[i*4], tan[i*4+1], tan[i*4+2]};
  }

  // Indices
  if (primitive.contains("indices"))
    geo.index = ReadAccessorAsUInts(primitive["indices"].get<int>());

  // Material
  Material mat;
  mat.shader_name = "pbr_shader";

  if (primitive.contains("material")) {
    int mat_index = primitive["material"].get<int>();
    if (doc.contains("materials") &&
        mat_index < static_cast<int>(doc["materials"].size())) {
      const json& jmat = doc["materials"][mat_index];

      if (jmat.contains("pbrMetallicRoughness")) {
        const json& pbr = jmat["pbrMetallicRoughness"];

        if (pbr.contains("baseColorFactor")) {
          const auto& f = pbr["baseColorFactor"];
          for (int k = 0; k < 4 && k < static_cast<int>(f.size()); ++k)
            mat.base_color[k] = f[k].get<float>();
        }
        if (pbr.contains("baseColorTexture"))
          mat.gl_albedo = ResolveTexture(pbr["baseColorTexture"]["index"].get<int>());

        mat.metallic_factor  = pbr.value("metallicFactor",  1.0f);
        mat.roughness_factor = pbr.value("roughnessFactor", 1.0f);

        if (pbr.contains("metallicRoughnessTexture"))
          mat.gl_metallic_roughness =
              ResolveTexture(pbr["metallicRoughnessTexture"]["index"].get<int>());
      }

      if (jmat.contains("normalTexture")) {
        const json& nt = jmat["normalTexture"];
        mat.gl_normal    = ResolveTexture(nt["index"].get<int>());
        mat.normal_scale = nt.value("scale", 1.0f);
      }

      if (jmat.contains("occlusionTexture")) {
        const json& ot = jmat["occlusionTexture"];
        mat.gl_occlusion       = ResolveTexture(ot["index"].get<int>());
        mat.occlusion_strength = ot.value("strength", 1.0f);
      }

      if (jmat.contains("emissiveTexture"))
        mat.gl_emissive =
            ResolveTexture(jmat["emissiveTexture"]["index"].get<int>());

      if (jmat.contains("emissiveFactor")) {
        const auto& ef = jmat["emissiveFactor"];
        for (int k = 0; k < 3 && k < static_cast<int>(ef.size()); ++k)
          mat.emissive_factor[k] = ef[k].get<float>();
      }

      std::string mat_name = jmat.value("name", "?");
      std::string alpha_str = jmat.value("alphaMode", "OPAQUE");
      if      (alpha_str == "MASK")  mat.alpha_mode = 1;
      else if (alpha_str == "BLEND") mat.alpha_mode = 2;
      else                           mat.alpha_mode = 0;

      mat.alpha_cutoff = jmat.value("alphaCutoff", 0.5f);

      // Hair cards need MASK (not BLEND) to show individual strands.
      // The glTF file incorrectly marks hair as BLEND; override it.
      std::string mat_name_lower = mat_name;
      std::transform(mat_name_lower.begin(), mat_name_lower.end(),
                     mat_name_lower.begin(), ::tolower);
      if (mat.alpha_mode == 2 &&
          (mat_name_lower.find("_hr_") != std::string::npos ||
           mat_name_lower.find("hair") != std::string::npos)) {
        mat.alpha_mode = 1;  // BLEND -> MASK
        mat.alpha_cutoff = 0.5f;
        LOG(ERROR) << "[GLTF Mat] Override hair BLEND->MASK: " << mat_name;
      }
      mat.double_sided = jmat.value("doubleSided", false);

      // ── KHR_materials_clearcoat ─────────────────────────────────────────
      if (jmat.contains("extensions")) {
        const json& ext = jmat["extensions"];

        if (ext.contains("KHR_materials_clearcoat")) {
          const json& cc = ext["KHR_materials_clearcoat"];
          mat.clearcoat_factor           = cc.value("clearcoatFactor", 0.0f);
          mat.clearcoat_roughness_factor = cc.value("clearcoatRoughnessFactor", 0.0f);
          if (cc.contains("clearcoatTexture"))
            mat.gl_clearcoat_tex =
                ResolveTexture(cc["clearcoatTexture"]["index"].get<int>());
          if (cc.contains("clearcoatRoughnessTexture"))
            mat.gl_clearcoat_roughness_tex =
                ResolveTexture(cc["clearcoatRoughnessTexture"]["index"].get<int>());
          if (cc.contains("clearcoatNormalTexture"))
            mat.gl_clearcoat_normal_tex =
                ResolveTexture(cc["clearcoatNormalTexture"]["index"].get<int>());
        }

        if (ext.contains("KHR_materials_specular")) {
          const json& sp = ext["KHR_materials_specular"];
          mat.specular_factor = sp.value("specularFactor", 1.0f);
          if (sp.contains("specularColorFactor")) {
            const auto& scf = sp["specularColorFactor"];
            for (int k = 0; k < 3 && k < static_cast<int>(scf.size()); ++k)
              mat.specular_color_factor[k] = scf[k].get<float>();
          }
          if (sp.contains("specularTexture"))
            mat.gl_specular_tex =
                ResolveTexture(sp["specularTexture"]["index"].get<int>());
          if (sp.contains("specularColorTexture"))
            mat.gl_specular_color_tex =
                ResolveTexture(sp["specularColorTexture"]["index"].get<int>());
        }
      }

      // Debug: log material info
      LOG(ERROR) << "[GLTF Mat] " << mat_name
                << " alpha=" << alpha_str
                << " cutoff=" << mat.alpha_cutoff
                << " albedo=" << mat.gl_albedo
                << " mr=" << mat.gl_metallic_roughness
                << " norm=" << mat.gl_normal
                << " base_color=[" << mat.base_color[0] << "," << mat.base_color[1]
                << "," << mat.base_color[2] << "," << mat.base_color[3] << "]";
    }
  }

  return std::make_unique<MeshObject>(geo, mat);
}

// ---------------------------------------------------------------------------
// Accessor readers
// ---------------------------------------------------------------------------
std::vector<float> GLTFLoader::ReadAccessorAsFloats(int idx) const {
  const json& doc = impl_->doc;
  if (!doc.contains("accessors") || idx < 0 ||
      idx >= static_cast<int>(doc["accessors"].size())) {
    LOG(ERROR) << "GLTFLoader: invalid accessor " << idx;
    return {};
  }
  const json& acc    = doc["accessors"][idx];
  int count          = acc["count"].get<int>();
  int component_type = acc["componentType"].get<int>();
  std::string type   = acc["type"].get<std::string>();
  int ncomp          = TypeNumComponents(type);
  int csize          = ComponentTypeSize(component_type);
  bool normalized    = acc.value("normalized", false);

  if (ncomp == 0 || csize == 0) { LOG(ERROR) << "bad accessor type"; return {}; }

  int bv_idx = acc.value("bufferView", -1);
  if (bv_idx < 0 || !doc.contains("bufferViews") ||
      bv_idx >= static_cast<int>(doc["bufferViews"].size())) {
    LOG(ERROR) << "GLTFLoader: accessor " << idx << " no bufferView";
    return {};
  }
  const json& bv = doc["bufferViews"][bv_idx];
  int bv_off     = bv.value("byteOffset", 0);
  int stride     = bv.value("byteStride", 0);
  int acc_off    = acc.value("byteOffset", 0);
  if (stride == 0) stride = ncomp * csize;

  const unsigned char* base = bin_buffer_.data() + bv_off + acc_off;

  std::vector<float> result;
  result.reserve(static_cast<size_t>(count) * static_cast<size_t>(ncomp));

  for (int i = 0; i < count; ++i) {
    const unsigned char* elem = base + i * stride;
    for (int c = 0; c < ncomp; ++c) {
      const unsigned char* p = elem + c * csize;
      float val = 0.0f;
      switch (component_type) {
        case 5120: { int8_t  v; std::memcpy(&v,&p[0],1);
          val = normalized ? std::max(v/127.0f,-1.0f) : (float)v; break; }
        case 5121: { uint8_t v; std::memcpy(&v,&p[0],1);
          val = normalized ? v/255.0f : (float)v; break; }
        case 5122: { int16_t v; std::memcpy(&v,&p[0],2);
          val = normalized ? std::max(v/32767.0f,-1.0f) : (float)v; break; }
        case 5123: { uint16_t v; std::memcpy(&v,&p[0],2);
          val = normalized ? v/65535.0f : (float)v; break; }
        case 5125: { uint32_t v; std::memcpy(&v,&p[0],4);
          val = (float)v; break; }
        case 5126: { std::memcpy(&val,&p[0],4); break; }
        default: break;
      }
      result.push_back(val);
    }
  }
  return result;
}

std::vector<unsigned int> GLTFLoader::ReadAccessorAsUInts(int idx) const {
  const json& doc = impl_->doc;
  if (!doc.contains("accessors") || idx < 0 ||
      idx >= static_cast<int>(doc["accessors"].size())) {
    LOG(ERROR) << "GLTFLoader: invalid accessor " << idx;
    return {};
  }
  const json& acc  = doc["accessors"][idx];
  int count        = acc["count"].get<int>();
  int ctype        = acc["componentType"].get<int>();
  int csize        = ComponentTypeSize(ctype);

  int bv_idx = acc.value("bufferView", -1);
  if (bv_idx < 0 || !doc.contains("bufferViews") ||
      bv_idx >= static_cast<int>(doc["bufferViews"].size())) {
    LOG(ERROR) << "GLTFLoader: index accessor no bufferView";
    return {};
  }
  const json& bv = doc["bufferViews"][bv_idx];
  int bv_off     = bv.value("byteOffset", 0);
  int stride     = bv.value("byteStride", csize);
  if (stride == 0) stride = csize;
  int acc_off    = acc.value("byteOffset", 0);

  const unsigned char* base = bin_buffer_.data() + bv_off + acc_off;
  std::vector<unsigned int> result;
  result.reserve(static_cast<size_t>(count));

  for (int i = 0; i < count; ++i) {
    const unsigned char* p = base + i * stride;
    unsigned int val = 0u;
    switch (ctype) {
      case 5121: { uint8_t  v; std::memcpy(&v,p,1); val=v; break; }
      case 5123: { uint16_t v; std::memcpy(&v,p,2); val=v; break; }
      case 5125: { std::memcpy(&val,p,4); break; }
      default:
        LOG(ERROR) << "GLTFLoader: unsupported index componentType " << ctype;
        break;
    }
    result.push_back(val);
  }
  return result;
}

// ---------------------------------------------------------------------------
// Texture helpers
// ---------------------------------------------------------------------------
unsigned int GLTFLoader::ResolveTexture(int texture_index) {
  const json& doc = impl_->doc;
  if (!doc.contains("textures") || texture_index < 0 ||
      texture_index >= static_cast<int>(doc["textures"].size())) {
    LOG(ERROR) << "GLTFLoader: invalid texture index " << texture_index;
    return 0u;
  }
  const json& tex   = doc["textures"][texture_index];
  int image_index   = tex.value("source", -1);
  int sampler_index = tex.value("sampler", -1);

  if (image_index < 0 ||
      image_index >= static_cast<int>(gl_texture_cache_.size())) {
    LOG(ERROR) << "GLTFLoader: texture " << texture_index
               << " bad image source " << image_index;
    return 0u;
  }
  if (gl_texture_cache_[image_index] != 0u)
    return gl_texture_cache_[image_index];

  unsigned int gl_id = LoadGLTexture(image_index, sampler_index);
  gl_texture_cache_[image_index] = gl_id;
  return gl_id;
}

unsigned int GLTFLoader::LoadGLTexture(int image_index, int sampler_index) const {
  const json& doc = impl_->doc;
  if (!doc.contains("images") || image_index < 0 ||
      image_index >= static_cast<int>(doc["images"].size())) {
    LOG(ERROR) << "GLTFLoader: invalid image index " << image_index;
    return 0u;
  }
  const json& image = doc["images"][image_index];
  int bv_idx        = image.value("bufferView", -1);
  if (bv_idx < 0 || !doc.contains("bufferViews") ||
      bv_idx >= static_cast<int>(doc["bufferViews"].size())) {
    LOG(ERROR) << "GLTFLoader: image " << image_index
               << " has no embedded bufferView";
    return 0u;
  }
  const json& bv  = doc["bufferViews"][bv_idx];
  int byte_offset = bv.value("byteOffset", 0);
  int byte_length = bv["byteLength"].get<int>();

  if (byte_offset + byte_length > static_cast<int>(bin_buffer_.size())) {
    LOG(ERROR) << "GLTFLoader: image bufferView out of bounds";
    return 0u;
  }

  stbi_set_flip_vertically_on_load(0);
  int w = 0, h = 0, ch = 0;
  // Force RGBA so alpha channel is always available for MASK/BLEND materials
  unsigned char* pixels = stbi_load_from_memory(
      bin_buffer_.data() + byte_offset, byte_length, &w, &h, &ch, 4);
  if (!pixels) {
    LOG(ERROR) << "GLTFLoader: stbi failed for image " << image_index
               << ": " << stbi_failure_reason();
    return 0u;
  }
  LOG(ERROR) << "[GLTF Tex] image=" << image_index
             << " orig_ch=" << ch << " size=" << w << "x" << h;

  // Always RGBA since we forced 4 channels above
  GLenum ifmt = GL_RGBA8, fmt = GL_RGBA;
  ch = 4;

  unsigned int tex_id = 0u;
  glGenTextures(1, &tex_id);
  glBindTexture(GL_TEXTURE_2D, tex_id);
  glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(ifmt),
               w, h, 0, fmt, GL_UNSIGNED_BYTE, pixels);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(pixels);

  GLint wrap_s = GL_REPEAT, wrap_t = GL_REPEAT;
  GLint min_f  = GL_LINEAR_MIPMAP_LINEAR, mag_f = GL_LINEAR;

  if (sampler_index >= 0 && doc.contains("samplers") &&
      sampler_index < static_cast<int>(doc["samplers"].size())) {
    const json& s = doc["samplers"][sampler_index];
    if (s.contains("wrapS"))    wrap_s = GltfWrapToGL(s["wrapS"].get<int>());
    if (s.contains("wrapT"))    wrap_t = GltfWrapToGL(s["wrapT"].get<int>());
    if (s.contains("minFilter")) min_f = GltfFilterToGL(s["minFilter"].get<int>(), true);
    if (s.contains("magFilter")) mag_f = GltfFilterToGL(s["magFilter"].get<int>(), false);
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     wrap_s);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     wrap_t);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_f);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_f);
  glBindTexture(GL_TEXTURE_2D, 0);

  return tex_id;
}
