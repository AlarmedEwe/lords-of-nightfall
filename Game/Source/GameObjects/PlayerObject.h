#ifndef PLAYER_OBJECT_H
#define PLAYER_OBJECT_H

#include "../GameObject.h"
#include "../LONModel.h"
#include "../Inventory.h"
#include "../Components/CharacterStats.h"
#include <string>

enum class PlayerAnimState
{
    Idle,
    Walk,
    Run,
    Jump,
    Attack
};

struct PlayerObject : public GameObject
{
    JPH::CharacterVirtual* character = nullptr;
    JPH::CharacterVirtualSettings* characterSettings = nullptr;

    float moveSpeed = 8.0f;
    float runSpeed = 13.0f;
    float jumpSpeed = 10.0f;
    Vector3 velocity = {0.0f, 0.0f, 0.0f};
    bool onGround = false;

    // Ângulo da câmera e rotação visual do personagem
    float cameraYaw = 0.0f;
    float meshYaw = 180.0f;

    // Atributos RPG do jogador (encapsulados via CharacterStats)
    CharacterStats stats;
    float& health = stats.health;
    float& maxHealth = stats.maxHealth;
    float& xp = stats.xp;
    float& maxXp = stats.maxXp;
    int& level = stats.level;
    float& baseAttack = stats.baseAttack;
    float& baseDefense = stats.baseDefense;

    // Estado de Ataque
    bool isAttacking = false;
    float attackTimer = 0.0f;
    float attackDuration = 0.5f;
    bool hasDealtDamageThisAttack = false;

    // Inventário
    Inventory inventory;

    // Modelo 3D e Outfit do personagem
    LONModel characterModel;
    LONModel equippedArmorModel;
    bool isArmorModelLoaded = false;
    std::string currentOutfitModel = "Superhero_Male.lonmodel";
    std::string currentOutfitTexture = "T_Superhero_Male_Ligh.png";

    // Modelo 3D da Arma Equipada
    Model equippedWeaponModel = {};
    std::string loadedWeaponModelPath = "";
    bool isWeaponModelLoaded = false;

    PlayerAnimState animState = PlayerAnimState::Idle;
    float animTime = 0.0f;
    float hitFlashTimer = 0.0f;

    PlayerObject()
    {
        stats.baseAttack = 15.0f;
        stats.baseDefense = 5.0f;
        stats.xp = 25.0f;
    }

    void Init(Physics& physics) override;
    void Update(Physics& physics, float deltaTime) override;
    void Draw() override;
    void Shutdown(Physics& physics) override;

    void TakeDamage(float damage);
    void Heal(float amount);
    void AddXp(float amount);
    void SetOutfit(const std::string& modelFile, const std::string& textureFile);
    float GetTotalAttack() const;
    float GetTotalDefense() const;
    void PerformAttackSweep(std::vector<GameObject*>& objects, struct DamageFeedbackSystem* feedback = nullptr);
};

#endif // PLAYER_OBJECT_H
