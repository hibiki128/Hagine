#pragma once
#include "Model/ModelStructs.h"
#include <unordered_map>

namespace Hagine {
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
    ClosedCylinder,
    kCount,
};

class PrimitiveModel {
  private:
    /// ====================================================
    /// private method
    /// ====================================================

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

    static PrimitiveModel* GetInstance() {
        static PrimitiveModel instance;
        return &instance;
    }

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
    void CreateClosedCylinder();

  private:
    /// ===================================================
    /// private variaus
    /// ===================================================

    std::unordered_map<PrimitiveType, PrimitiveData> primitiveDataMap_;
};
} // namespace Hagine
