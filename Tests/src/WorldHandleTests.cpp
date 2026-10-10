#include <doctest/doctest.h>

#include "World.h"

#include <random>
#include <vector>

using namespace Crumbs2D;

// Tests written with Claude Opus 5.5
namespace
{
// Compare composante par composante : doctest affiche alors les valeurs
// exactes en cas d'échec (il ne sait pas afficher un glm::vec2).
void CheckVec2(glm::vec2 actual, glm::vec2 expected)
{
    CHECK(actual.x == expected.x);
    CHECK(actual.y == expected.y);
}
} // namespace

TEST_CASE("Un body créé est valide, un Body par défaut ne l'est pas")
{
    World world;
    const Body body = world.AddBody();
    const Body empty;

    CHECK(body.IsValid());
    CHECK_FALSE(empty.IsValid());
}

TEST_CASE("Chaque body garde ses propres données")
{
    World world;
    std::vector<Body> bodies;

    for (int i = 0; i < 10; ++i)
    {
        Body body = world.AddBody();
        body.SetPosition({static_cast<float>(i), static_cast<float>(-i)});
        body.SetVelocity({static_cast<float>(i * 10), 0.0f});
        bodies.push_back(body);
    }

    for (int i = 0; i < 10; ++i)
    {
        CheckVec2(bodies[i].GetPosition(), {static_cast<float>(i), static_cast<float>(-i)});
        CheckVec2(bodies[i].GetVelocity(), {static_cast<float>(i * 10), 0.0f});
    }
}

TEST_CASE("Supprimer un body au milieu ne modifie pas les autres")
{
    World world;
    Body a = world.AddBody();
    Body b = world.AddBody();
    Body c = world.AddBody();

    a.SetPosition({1.0f, 1.0f});
    b.SetPosition({2.0f, 2.0f});
    c.SetPosition({3.0f, 3.0f});
    c.SetVelocity({30.0f, 0.0f});

    // b est au milieu : c (le dernier) est déplacé à sa place par le swap and pop.
    world.DeleteBody(b);

    CHECK(a.IsValid());
    CHECK_FALSE(b.IsValid());
    CHECK(c.IsValid());

    CheckVec2(a.GetPosition(), {1.0f, 1.0f});
    CheckVec2(c.GetPosition(), {3.0f, 3.0f});
    CheckVec2(c.GetVelocity(), {30.0f, 0.0f});
}

TEST_CASE("Supprimer le dernier body fonctionne")
{
    World world;
    Body a = world.AddBody();
    Body b = world.AddBody();

    a.SetPosition({1.0f, 1.0f});
    world.DeleteBody(b);

    CHECK(a.IsValid());
    CHECK_FALSE(b.IsValid());
    CheckVec2(a.GetPosition(), {1.0f, 1.0f});
}

TEST_CASE("Un handle recyclé n'est pas confondu avec l'ancien")
{
    World world;
    Body oldBody = world.AddBody();
    world.DeleteBody(oldBody);

    // Le nouveau body réutilise la case libérée, avec une génération différente.
    Body newBody = world.AddBody();
    newBody.SetPosition({5.0f, 5.0f});

    CHECK(newBody.IsValid());
    CHECK_FALSE(oldBody.IsValid());
    CheckVec2(newBody.GetPosition(), {5.0f, 5.0f});
}

TEST_CASE("Un body d'un autre world n'est pas valide dans celui-ci")
{
    World worldA;
    World worldB;
    const Body bodyA = worldA.AddBody();
    worldB.AddBody();

    // bodyA a le même handle que le body de worldB, mais pas le même world.
    CHECK(bodyA.IsValid());
    worldA.DeleteBody(bodyA);
    CHECK_FALSE(bodyA.IsValid());
}

TEST_CASE("Séquence aléatoire de créations et de suppressions")
{
    struct Reference
    {
        Body body;
        glm::vec2 position;
        glm::vec2 velocity;
    };

    World world;
    std::vector<Reference> alive;
    std::vector<Body> dead;

    // Graine fixe : un échec est toujours reproductible.
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> action(0, 99);
    std::uniform_real_distribution<float> value(-100.0f, 100.0f);

    for (int step = 0; step < 3000; ++step)
    {
        // Un peu plus de créations que de suppressions, pour faire varier la taille.
        const bool create = alive.empty() || action(rng) < 55;

        if (create)
        {
            Reference ref{world.AddBody(), {value(rng), value(rng)}, {value(rng), value(rng)}};
            ref.body.SetPosition(ref.position);
            ref.body.SetVelocity(ref.velocity);
            alive.push_back(ref);
        }
        else
        {
            std::uniform_int_distribution<std::size_t> pick(0, alive.size() - 1);
            const std::size_t i = pick(rng);

            world.DeleteBody(alive[i].body);
            dead.push_back(alive[i].body);

            alive[i] = alive.back();
            alive.pop_back();
        }

        // Tous les vivants doivent avoir conservé exactement leurs données.
        for (const Reference& ref : alive)
        {
            REQUIRE(ref.body.IsValid());
            CheckVec2(ref.body.GetPosition(), ref.position);
            CheckVec2(ref.body.GetVelocity(), ref.velocity);
        }

        // Tous les supprimés doivent rester invalides, même si leur case a été recyclée.
        for (const Body& body : dead)
            REQUIRE_FALSE(body.IsValid());
    }
}
TEST_CASE("Un body recyclé ne reprend pas les valeurs de l'ancien")
{
    // Donne des valeurs non nulles à tous les champs, y compris une force en attente.
    auto fill = [](Body body) {
        body.SetPosition({7.0f, -3.0f});
        body.SetRotation(1.5f);
        body.SetVelocity({4.0f, 2.0f});
        body.SetAngularVelocity(9.0f);
        body.SetMass(5.0f);
        body.AddForce({100.0f, 50.0f});
    };

    // Vérifie que tous les champs ont leur valeur par défaut.
    auto checkDefaults = [](World& world, Body body) {
        CheckVec2(body.GetPosition(), {0.0f, 0.0f});
        CHECK(body.GetRotation() == 0.0f);
        CheckVec2(body.GetVelocity(), {0.0f, 0.0f});
        CHECK(body.GetAngularVelocity() == 0.0f);
        CHECK(body.GetMass() == 1.0f);

        // Les forces ne sont pas lisibles : sans gravité, un Step ne doit rien bouger.
        world.Step(1.0f / 60.0f);
        CheckVec2(body.GetVelocity(), {0.0f, 0.0f});
        CHECK(body.GetAngularVelocity() == 0.0f);
    };

    SUBCASE("Le body supprimé était le dernier")
    {
        World world;
        Body old = world.AddBody();
        fill(old);
        world.DeleteBody(old);

        checkDefaults(world, world.AddBody());
    }

    SUBCASE("Le body supprimé était au milieu")
    {
        World world;
        Body first = world.AddBody();
        Body middle = world.AddBody();
        Body last = world.AddBody();
        fill(first);
        fill(middle);
        fill(last);

        world.DeleteBody(middle);
        world.DeleteBody(last); // libère aussi la case de fin, avec ses anciennes valeurs

        checkDefaults(world, world.AddBody());
    }
}