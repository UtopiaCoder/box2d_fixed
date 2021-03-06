/*
* Copyright (c) 2009 Erin Catto http://www.box2d.org
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

#ifndef DYNAMIC_TREE_TEST_H
#define DYNAMIC_TREE_TEST_H

class DynamicTreeTest : public Test
{
public:

	enum
	{
		e_actorCount = 128
	};

	DynamicTreeTest()
	{
		m_worldExtent = 15_fx;
		m_proxyExtent = "0.5"_fx;

		srand(888);

		for (int32 i = 0; i < e_actorCount; ++i)
		{
			Actor* actor = m_actors + i;
			GetRandomAABB(&actor->aabb);
			actor->proxyId = m_tree.CreateProxy(actor->aabb, actor);
		}

		m_stepCount = 0;

		float32 h = m_worldExtent;
		m_queryAABB.lowerBound.Set(-3_fx, -4_fx + h);
		m_queryAABB.upperBound.Set(5_fx, 6_fx + h);

		m_rayCastInput.p1.Set(-5_fx, 5_fx + h);
		m_rayCastInput.p2.Set(7_fx, -4_fx + h);
		//m_rayCastInput.p1.Set(0_fx, 2_fx + h);
		//m_rayCastInput.p2.Set(0_fx, -2_fx + h);
		m_rayCastInput.maxFraction = 1_fx;

		m_automated = false;
	}

	static Test* Create()
	{
		return new DynamicTreeTest;
	}

	void Step(Settings* settings)
	{
		B2_NOT_USED(settings);

		m_rayActor = NULL;
		for (int32 i = 0; i < e_actorCount; ++i)
		{
			m_actors[i].fraction = 1_fx;
			m_actors[i].overlap = false;
		}

		if (m_automated == true)
		{
			int32 actionCount = b2Max(1, e_actorCount >> 2);

			for (int32 i = 0; i < actionCount; ++i)
			{
				Action();
			}
		}

		Query();
		RayCast();

		for (int32 i = 0; i < e_actorCount; ++i)
		{
			Actor* actor = m_actors + i;
			if (actor->proxyId == b2_nullNode)
				continue;

			b2Color c("0.9"_fx, "0.9"_fx, "0.9"_fx);
			if (actor == m_rayActor && actor->overlap)
			{
				c.Set("0.9"_fx, "0.6"_fx, "0.6"_fx);
			}
			else if (actor == m_rayActor)
			{
				c.Set("0.6"_fx, "0.9"_fx, "0.6"_fx);
			}
			else if (actor->overlap)
			{
				c.Set("0.6"_fx, "0.6"_fx, "0.9"_fx);
			}

			g_debugDraw.DrawAABB(&actor->aabb, c);
		}

		b2Color c("0.7"_fx, "0.7"_fx, "0.7"_fx);
		g_debugDraw.DrawAABB(&m_queryAABB, c);

		g_debugDraw.DrawSegment(m_rayCastInput.p1, m_rayCastInput.p2, c);

		b2Color c1("0.2"_fx, "0.9"_fx, "0.2"_fx);
		b2Color c2("0.9"_fx, "0.2"_fx, "0.2"_fx);
		g_debugDraw.DrawPoint(m_rayCastInput.p1, 6_fx, c1);
		g_debugDraw.DrawPoint(m_rayCastInput.p2, 6_fx, c2);

		if (m_rayActor)
		{
			b2Color cr("0.2"_fx, "0.2"_fx, "0.9"_fx);
			b2Vec2 p = m_rayCastInput.p1 + m_rayActor->fraction * (m_rayCastInput.p2 - m_rayCastInput.p1);
			g_debugDraw.DrawPoint(p, 6_fx, cr);
		}

		{
			int32 height = m_tree.GetHeight();
			g_debugDraw.DrawString(5, m_textLine, "dynamic tree height = %d", height);
			m_textLine += DRAW_STRING_NEW_LINE;
		}

		++m_stepCount;
	}

	void Keyboard(int key)
	{
		switch (key)
		{
		case GLFW_KEY_A:
			m_automated = !m_automated;
			break;

		case GLFW_KEY_C:
			CreateProxy();
			break;

		case GLFW_KEY_D:
			DestroyProxy();
			break;

		case GLFW_KEY_M:
			MoveProxy();
			break;
		}
	}

	bool QueryCallback(int32 proxyId)
	{
		Actor* actor = (Actor*)m_tree.GetUserData(proxyId);
		actor->overlap = b2TestOverlap(m_queryAABB, actor->aabb);
		return true;
	}

	float32 RayCastCallback(const b2RayCastInput& input, int32 proxyId)
	{
		Actor* actor = (Actor*)m_tree.GetUserData(proxyId);

		b2RayCastOutput output;
		bool hit = actor->aabb.RayCast(&output, input);

		if (hit)
		{
			m_rayCastOutput = output;
			m_rayActor = actor;
			m_rayActor->fraction = output.fraction;
			return output.fraction;
		}

		return input.maxFraction;
	}

private:

	struct Actor
	{
		b2AABB aabb;
		float32 fraction;
		bool overlap;
		int32 proxyId;
	};

	void GetRandomAABB(b2AABB* aabb)
	{
		b2Vec2 w; w.Set(2_fx * m_proxyExtent, 2_fx * m_proxyExtent);
		//aabb->lowerBound.x = -m_proxyExtent;
		//aabb->lowerBound.y = -m_proxyExtent + m_worldExtent;
		aabb->lowerBound.x = RandomFloat(-m_worldExtent, m_worldExtent);
		aabb->lowerBound.y = RandomFloat(0_fx, 2_fx * m_worldExtent);
		aabb->upperBound = aabb->lowerBound + w;
	}

	void MoveAABB(b2AABB* aabb)
	{
		b2Vec2 d;
		d.x = RandomFloat(-"0.5"_fx, "0.5"_fx);
		d.y = RandomFloat(-"0.5"_fx, "0.5"_fx);
		//d.x = 2_fx;
		//d.y = 0_fx;
		aabb->lowerBound += d;
		aabb->upperBound += d;

		b2Vec2 c0 = "0.5"_fx * (aabb->lowerBound + aabb->upperBound);
		b2Vec2 min; min.Set(-m_worldExtent, 0_fx);
		b2Vec2 max; max.Set(m_worldExtent, 2_fx * m_worldExtent);
		b2Vec2 c = b2Clamp(c0, min, max);

		aabb->lowerBound += c - c0;
		aabb->upperBound += c - c0;
	}

	void CreateProxy()
	{
		for (int32 i = 0; i < e_actorCount; ++i)
		{
			int32 j = rand() % e_actorCount;
			Actor* actor = m_actors + j;
			if (actor->proxyId == b2_nullNode)
			{
				GetRandomAABB(&actor->aabb);
				actor->proxyId = m_tree.CreateProxy(actor->aabb, actor);
				return;
			}
		}
	}

	void DestroyProxy()
	{
		for (int32 i = 0; i < e_actorCount; ++i)
		{
			int32 j = rand() % e_actorCount;
			Actor* actor = m_actors + j;
			if (actor->proxyId != b2_nullNode)
			{
				m_tree.DestroyProxy(actor->proxyId);
				actor->proxyId = b2_nullNode;
				return;
			}
		}
	}

	void MoveProxy()
	{
		for (int32 i = 0; i < e_actorCount; ++i)
		{
			int32 j = rand() % e_actorCount;
			Actor* actor = m_actors + j;
			if (actor->proxyId == b2_nullNode)
			{
				continue;
			}

			b2AABB aabb0 = actor->aabb;
			MoveAABB(&actor->aabb);
			b2Vec2 displacement = actor->aabb.GetCenter() - aabb0.GetCenter();
			m_tree.MoveProxy(actor->proxyId, actor->aabb, displacement);
			return;
		}
	}

	void Action()
	{
		int32 choice = rand() % 20;

		switch (choice)
		{
		case 0:
			CreateProxy();
			break;

		case 1:
			DestroyProxy();
			break;

		default:
			MoveProxy();
		}
	}

	void Query()
	{
		m_tree.Query(this, m_queryAABB);

		for (int32 i = 0; i < e_actorCount; ++i)
		{
			if (m_actors[i].proxyId == b2_nullNode)
			{
				continue;
			}

			bool overlap = b2TestOverlap(m_queryAABB, m_actors[i].aabb);
			B2_NOT_USED(overlap);
			b2Assert(overlap == m_actors[i].overlap);
		}
	}

	void RayCast()
	{
		m_rayActor = NULL;

		b2RayCastInput input = m_rayCastInput;

		// Ray cast against the dynamic tree.
		m_tree.RayCast(this, input);

		// Brute force ray cast.
		Actor* bruteActor = NULL;
		b2RayCastOutput bruteOutput;
		for (int32 i = 0; i < e_actorCount; ++i)
		{
			if (m_actors[i].proxyId == b2_nullNode)
			{
				continue;
			}

			b2RayCastOutput output;
			bool hit = m_actors[i].aabb.RayCast(&output, input);
			if (hit)
			{
				bruteActor = m_actors + i;
				bruteOutput = output;
				input.maxFraction = output.fraction;
			}
		}

		if (bruteActor != NULL)
		{
			b2Assert(bruteOutput.fraction == m_rayCastOutput.fraction);
		}
	}

	float32 m_worldExtent;
	float32 m_proxyExtent;

	b2DynamicTree m_tree;
	b2AABB m_queryAABB;
	b2RayCastInput m_rayCastInput;
	b2RayCastOutput m_rayCastOutput;
	Actor* m_rayActor;
	Actor m_actors[e_actorCount];
	int32 m_stepCount;
	bool m_automated;
};

#endif
