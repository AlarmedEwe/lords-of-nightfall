#ifndef LON_TERRAIN_ALIGNER_H
#define LON_TERRAIN_ALIGNER_H

#include "raylib.h"
#include "Scene.h"
#include "GameObjects/TerrainObject.h"
#include "Physics.h"

// Classe com Responsabilidade Única (SRP):
// Responsável exclusivamente por alinhar entidades e seus componentes físicos
// à elevação do terreno (Ground Snapping / Height Synchronization).
class TerrainAligner
{
public:
    // Alinha um único GameObject à superfície do terreno
    static void AlignObject(GameObject* obj, TerrainObject& terrain, Physics& physics);

    // Alinha todos os GameObjects contidos dentro de um raio cilíndrico (centerX, centerZ, radius)
    static void AlignAllInRadius(Scene& scene, TerrainObject& terrain, Physics& physics, float centerX, float centerZ, float radius);

    // Alinha todos os GameObjects da cena
    static void AlignAll(Scene& scene, TerrainObject& terrain, Physics& physics);
};

#endif // LON_TERRAIN_ALIGNER_H
