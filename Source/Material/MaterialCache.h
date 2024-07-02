#pragma once
#include <Common/IdTypes.h>
#include "Material.h"
#include <vector>

class MaterialCache
{
public:
    [[nodiscard]] MaterialId add_material(const Material& material);
    [[nodiscard]] const Material& get_material(MaterialId id) const;
    

private:
    void cleanup(); // TODO: Should this call image cleanups? Or image cache should manage memory itself? (probably latter)
    std::vector<Material> m_materials;
};