#include "PhysicsWorld.h"

static const short GROUP_GROUND = 1 << 0;
static const short GROUP_CHUNK  = 1 << 1;

PhysicsWorld::PhysicsWorld() {
  broadphase = new btDbvtBroadphase();
  config = new btDefaultCollisionConfiguration();
  dispatcher = new btCollisionDispatcher(config);
  solver = new btSequentialImpulseConstraintSolver();

  world = new btDiscreteDynamicsWorld(
    dispatcher, broadphase, solver, config
  );

  world->setGravity(btVector3(0, -9.81f, 0));
}

PhysicsWorld::~PhysicsWorld() {
  for (auto& c : chunks) {
    world->removeRigidBody(c.body);
    delete c.body;
    delete c.shape;
  }
  delete world;
  delete solver;
  delete dispatcher;
  delete config;
  delete broadphase;
}

void PhysicsWorld::addGround() {
  float groundY = -0.55f;
  auto* shape = new btStaticPlaneShape(btVector3(0,1,0), -groundY);
  auto* motion = new btDefaultMotionState();
  btRigidBody::btRigidBodyConstructionInfo info(0, motion, shape);
  auto* body = new btRigidBody(info);
  world->addRigidBody(body, GROUP_GROUND, GROUP_CHUNK);
}

PhysicsChunk PhysicsWorld::addChunk(const glm::vec3& pos,
                                    const glm::vec3& halfExt,
                                    float mass) {
  auto* shape = new btBoxShape(btVector3(
    halfExt.x, halfExt.y, halfExt.z
  ));

  btTransform t;
  t.setIdentity();
  t.setOrigin(btVector3(pos.x, pos.y, pos.z));

  btVector3 inertia(0,0,0);
  shape->calculateLocalInertia(mass, inertia);

  auto* motion = new btDefaultMotionState(t);
  btRigidBody::btRigidBodyConstructionInfo info(
    mass, motion, shape, inertia
  );

  auto* body = new btRigidBody(info);
  body->setRestitution(0.1f);
  body->setFriction(0.8f);
  shape->setMargin(0.001f);

  world->addRigidBody(body, GROUP_CHUNK, GROUP_GROUND);

  PhysicsChunk c;
  c.body = body;
  c.shape = shape;

  chunks.push_back(c);
  return c;
}

void PhysicsWorld::step(float dt) {
  world->stepSimulation(dt, 5);
}
