/*
* Copyright (c) 2008-2009 Erin Catto http://www.box2d.org
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef SENSOR_TEST_H
#define SENSOR_TEST_H

// This is used to test sensor shapes.
class SensorTest : public Test
{
public:

	enum
	{
		e_count = 7
	};

	SensorTest()
	{
		{
			b2BodyDef bd;
			b2Body* ground = m_world->CreateBody(&bd);

			{
				b2EdgeShape shape;
				shape.Set(b2Vec2(-40_fx, 0_fx), b2Vec2(40_fx, 0_fx));
				ground->CreateFixture(&shape, 0_fx);
			}

#if 0
			{
				b2FixtureDef sd;
				sd.SetAsBox(10_fx, 2_fx, b2Vec2(0_fx, 20_fx), 0_fx);
				sd.isSensor = true;
				m_sensor = ground->CreateFixture(&sd);
			}
#else
			{
				b2CircleShape shape;
				shape.m_radius = 5_fx;
				shape.m_p.Set(0_fx, 10_fx);

				b2FixtureDef fd;
				fd.shape = &shape;
				fd.isSensor = true;
				m_sensor = ground->CreateFixture(&fd);
			}
#endif
		}

		{
			b2CircleShape shape;
			shape.m_radius = 1_fx;

			for (int32 i = 0; i < e_count; ++i)
			{
				b2BodyDef bd;
				bd.type = b2_dynamicBody;
				bd.position.Set(-10_fx + 3_fx * (float32)i, 20_fx);
				bd.userData = m_touching + i;

				m_touching[i] = false;
				m_bodies[i] = m_world->CreateBody(&bd);

				m_bodies[i]->CreateFixture(&shape, 1_fx);
			}
		}
	}

	// Implement contact listener.
	void BeginContact(b2Contact* contact)
	{
		b2Fixture* fixtureA = contact->GetFixtureA();
		b2Fixture* fixtureB = contact->GetFixtureB();

		if (fixtureA == m_sensor)
		{
			void* userData = fixtureB->GetBody()->GetUserData();
			if (userData)
			{
				bool* touching = (bool*)userData;
				*touching = true;
			}
		}

		if (fixtureB == m_sensor)
		{
			void* userData = fixtureA->GetBody()->GetUserData();
			if (userData)
			{
				bool* touching = (bool*)userData;
				*touching = true;
			}
		}
	}

	// Implement contact listener.
	void EndContact(b2Contact* contact)
	{
		b2Fixture* fixtureA = contact->GetFixtureA();
		b2Fixture* fixtureB = contact->GetFixtureB();

		if (fixtureA == m_sensor)
		{
			void* userData = fixtureB->GetBody()->GetUserData();
			if (userData)
			{
				bool* touching = (bool*)userData;
				*touching = false;
			}
		}

		if (fixtureB == m_sensor)
		{
			void* userData = fixtureA->GetBody()->GetUserData();
			if (userData)
			{
				bool* touching = (bool*)userData;
				*touching = false;
			}
		}
	}

	void Step(Settings* settings)
	{
		Test::Step(settings);

		// Traverse the contact results. Apply a force on shapes
		// that overlap the sensor.
		for (int32 i = 0; i < e_count; ++i)
		{
			if (m_touching[i] == false)
			{
				continue;
			}

			b2Body* body = m_bodies[i];
			b2Body* ground = m_sensor->GetBody();

			b2CircleShape* circle = (b2CircleShape*)m_sensor->GetShape();
			b2Vec2 center = ground->GetWorldPoint(circle->m_p);

			b2Vec2 position = body->GetPosition();

			b2Vec2 d = center - position;
			if (d.LengthSquared() < fixed_number::epsilon * fixed_number::epsilon)
			{
				continue;
			}

			d.Normalize();
			b2Vec2 F = 100_fx * d;
			body->ApplyForce(F, position, false);
		}
	}

	static Test* Create()
	{
		return new SensorTest;
	}

	b2Fixture* m_sensor;
	b2Body* m_bodies[e_count];
	bool m_touching[e_count];
};

#endif
