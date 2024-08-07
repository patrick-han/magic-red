#include "MaterialCache.h"


[[nodiscard]] MaterialId MaterialCache::add_material(const Material& material) {
    const MaterialId materialId = static_cast<uint32_t>(m_materials.size());
    m_materials.push_back(material);
    return materialId;
}

[[nodiscard]] const Material& MaterialCache::get_material(MaterialId id) const {
    return m_materials[id];
}

[[nodiscard]] const void* MaterialCache::get_material_data() const {
    return m_materials.data();
}

[[nodiscard]] int MaterialCache::get_material_count() const {
    return static_cast<uint32_t>(m_materials.size());
}
