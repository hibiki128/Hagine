#pragma once
#include "Model/ModelStructs.h"
#include <unordered_map>

enum class PrimitiveType {
    Plane,
    Sphere,
    Cube,
    Cylinder,
    Ring,
};
class PrimitiveModel {
  private:
    struct PrimitiveData {
        std::vector<VertexData> vertices;
        std::vector<uint32_t> indices;
        Matrix4x4 uvMatrix;
        Vector4 color;
    };

  public:
    void Initialize();
    void CreateSphere();
    void CreatePlane();
    void CreateCube();
    void CreateCylinder();
    void CreateRing();

    PrimitiveData GetPrimitiveData(const PrimitiveType& type) {
        auto it = primitiveDataMap_.find(type);
        if (it != primitiveDataMap_.end()) {
            return it->second;
        }
        return {};
    }

  private:
    std::unordered_map<PrimitiveType, PrimitiveData> primitiveDataMap_;
};
