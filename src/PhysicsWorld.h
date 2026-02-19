#pragma once
#include <btBulletDynamicsCommon.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct PhysicsChunk {
  btRigidBody* body;
  btCollisionShape* shape;

  glm::mat4 originalTransform;
};

class PhysicsWorld {
public:
  PhysicsWorld();
  ~PhysicsWorld();

  void step(float dt);

  void addGround();
  PhysicsChunk addChunk(const glm::vec3& pos,
                         const glm::vec3& halfExtents,
                         float mass);

  std::vector<PhysicsChunk> chunks;

private:
  btBroadphaseInterface* broadphase;
  btDefaultCollisionConfiguration* config;
  btCollisionDispatcher* dispatcher;
  btSequentialImpulseConstraintSolver* solver;
  btDiscreteDynamicsWorld* world;
};
