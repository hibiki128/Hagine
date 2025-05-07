#pragma once
#include "Model/ModelStructs.h"
#include <unordered_map>

enum class PrimitiveType {
    None = 0,
    Plane,
    Sphere,
    Cube,
    Cylinder,
    Ring,
    Triangle,
    Cone,
    Pyramid,
};


class PrimitiveModel {
  private:
    /// ====================================================
    /// private method
    /// ====================================================

    static PrimitiveModel *instance;

    PrimitiveModel() = default;
    ~PrimitiveModel() = default;
    PrimitiveModel(PrimitiveModel &) = delete;
    PrimitiveModel &operator=(PrimitiveModel &) = delete;

    struct PrimitiveData {
        std::vector<VertexData> vertices;
        std::vector<uint32_t> indices;
        Matrix4x4 uvMatrix;
        Vector4 color;
    };

  public:
    /// =============================================================
    /// public method
    /// =============================================================

    void Initialize();

    static PrimitiveModel *GetInstance();

    void Finalize();

    PrimitiveData GetPrimitiveData(const PrimitiveType &type) {
        auto it = primitiveDataMap_.find(type);
        if (it != primitiveDataMap_.end()) {
            return it->second;
        }
        return {};
    }

  private:
    void CreateSphere();
    void CreatePlane();
    void CreateCube();
    void CreateCylinder();
    void CreateRing();
    void CreateTriangle();
    void CreateCone();
    void CreatePyramid();

  private:
    /// ===================================================
    /// private variaus
    /// ===================================================

    std::unordered_map<PrimitiveType, PrimitiveData> primitiveDataMap_;
};
