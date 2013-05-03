#pragma once

#include "system.h"

#include <SFML/System.hpp>
#include <BULLET/btBulletDynamicsCommon.h>
#include <GLM/glm.hpp>
#include <GLEW/glew.h>


class ModulePhysic : public Module
{
	// general
	sf::Clock clock;
	void Init();
	~ModulePhysic();
	void Update();
	void Listeners();

	// bullet
	btBroadphaseInterface* broadphase;
	btDefaultCollisionConfiguration* configuration;
	btCollisionDispatcher* dispatcher;
	btSequentialImpulseConstraintSolver* solver;
	btDiscreteDynamicsWorld* world;

	// helper
	btVector3 ModulePhysic::Position(glm::vec3 &Coordinates);
	glm::vec3 ModulePhysic::Position(btVector3 &Coordinates);
	btQuaternion Rotation(glm::vec3 &Angles);
	glm::vec3 Rotation(btQuaternion &Quaternion);

	// debug
	class DebugDrawer : public btIDebugDraw
	{
	public:
		DebugDrawer(ManagerEntity *Entity, ManagerGlobal *Global, HelperFile *File, HelperDebug *Debug);
		void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color);
		void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) {}
		void reportErrorWarning(const char *warningString) {}
		void draw3dText(const btVector3 &location, const char *textString) {}
		void setDebugMode(int debugMode);
		int	getDebugMode() const;
	private:
		ManagerEntity *Entity; ManagerGlobal *Global; HelperFile *File; HelperDebug *Debug;
		GLuint shader;
		int mode;
	};

	// transform
	void Matrix(unsigned int id);
};