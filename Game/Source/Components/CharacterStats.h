#ifndef CHARACTER_STATS_H
#define CHARACTER_STATS_H

#include <algorithm>

struct CharacterStats
{
    float health = 100.0f;
    float maxHealth = 100.0f;
    float xp = 0.0f;
    float maxXp = 100.0f;
    int level = 1;

    float baseAttack = 10.0f;
    float baseDefense = 2.0f;

    inline bool IsDead() const { return health <= 0.0f; }

    inline void Heal(float amount)
    {
        health = std::min(maxHealth, health + amount);
    }

    inline bool TakeDamage(float amount)
    {
        float netDamage = std::max(1.0f, amount - baseDefense);
        health = std::max(0.0f, health - netDamage);
        return IsDead();
    }

    inline bool AddXp(float amount)
    {
        xp += amount;
        bool leveledUp = false;
        while (xp >= maxXp)
        {
            xp -= maxXp;
            level++;
            maxXp *= 1.25f;
            maxHealth += 20.0f;
            health = maxHealth;
            baseAttack += 5.0f;
            baseDefense += 2.0f;
            leveledUp = true;
        }
        return leveledUp;
    }
};

#endif // CHARACTER_STATS_H
