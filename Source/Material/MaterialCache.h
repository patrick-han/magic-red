#pragma once
#include <Common/IdTypes.h>
#include "Material.h"
#include <vector>

class MaterialCache
{
public:
    MaterialCache() = default;
    ~MaterialCache() = default;
    MaterialCache(const MaterialCache&) = delete;
    MaterialCache& operator=(const MaterialCache&) = delete;
    MaterialCache(MaterialCache&&) = delete;
    MaterialCache& operator=(MaterialCache&&) = delete;

    [[nodiscard]] MaterialId add_material(const Material& material);
    [[nodiscard]] const Material& get_material(MaterialId id) const;
    [[nodiscard]] const void* get_material_data() const;
    [[nodiscard]] int get_material_count() const;

private:
    void cleanup(); // TODO: Should this call image cleanups? Or image cache should manage memory itself? (probably latter)
    std::vector<Material> m_materials;
};