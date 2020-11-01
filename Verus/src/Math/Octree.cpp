// Copyright (C) 2021, Dmitry Maluev (dmaluev@gmail.com). All rights reserved.
#include "verus.h"

using namespace verus;
using namespace verus::Math;

// Octree::Node:

Octree::Node::Node()
{
}

Octree::Node::~Node()
{
}

int Octree::Node::GetChildIndex(int currentNode, int child)
{
	return 1 + (currentNode << 3) + child;
}

bool Octree::Node::HasChildren(int currentNode, int nodeCount)
{
	return GetChildIndex(currentNode, 7) < nodeCount;
}

void Octree::Node::BindClient(RcClient client)
{
	_vClients.push_back(client);
}

void Octree::Node::UnbindClient(void* pToken)
{
	VERUS_WHILE(Vector<Client>, _vClients, it)
	{
		if (pToken == (*it)._pToken)
			it = _vClients.erase(it);
		else
			it++;
	}
}

void Octree::Node::UpdateDynamicClient(RcClient client)
{
	for (auto& x : _vClients)
	{
		if (x._pToken == client._pToken)
		{
			x = client;
			return;
		}
	}
}

// Octree:

Octree::Octree()
{
}

Octree::~Octree()
{
	Done();
}

void Octree::Init(RcBounds bounds, RcVector3 limit)
{
	VERUS_INIT();

	_bounds = bounds;
	_limit = limit;

	Build();
}

void Octree::Done()
{
	VERUS_DONE(Octree);
}

void Octree::Build(int currentNode, int depth)
{
	const Vector3 dim = _bounds.GetDimensions();
	const Vector3 ratio = VMath::divPerElem(dim, _limit);
	const int maxDepthX = Math::HighestBit(static_cast<int>(ratio.getX()));
	const int maxDepthY = Math::HighestBit(static_cast<int>(ratio.getY()));
	const int maxDepthZ = Math::HighestBit(static_cast<int>(ratio.getZ()));
	const int maxDepth = Math::Max(Math::Max(maxDepthX, maxDepthY), maxDepthZ);
	if (!currentNode)
	{
		const int depthCount = maxDepth + 1;
		int nodeCount = 0;
		VERUS_FOR(i, depthCount)
		{
			nodeCount +=
				(1 << Math::Min(i, maxDepthX)) *
				(1 << Math::Min(i, maxDepthY)) *
				(1 << Math::Min(i, maxDepthZ));
		}
		_vNodes.resize(nodeCount);
		_vNodes[0].SetBounds(_bounds);
	}

	if (depth < maxDepth) // Has children?
	{
		VERUS_FOR(i, 8)
		{
			RcBounds currentBounds = _vNodes[currentNode].GetBounds();

			Bounds bounds;
			currentBounds.GetQuadrant3D(i, bounds);

			if (depth >= maxDepthX)
			{
				bounds.Set(currentBounds, 0);
				if ((i >> 0) & 0x1)
					continue;
			}
			if (depth >= maxDepthY)
			{
				bounds.Set(currentBounds, 1);
				if ((i >> 1) & 0x1)
					continue;
			}
			if (depth >= maxDepthZ)
			{
				bounds.Set(currentBounds, 2);
				if ((i >> 2) & 0x1)
					continue;
			}

			const int childIndex = Node::GetChildIndex(currentNode, i);
			_vNodes[childIndex].SetBounds(bounds);
			Build(childIndex, depth + 1);
		}
	}
}

bool Octree::BindClient(RcClient client, bool forceRoot, int currentNode)
{
	if (!currentNode)
	{
		if (_vNodes.empty())
			return false; // Octree is not ready.
		UnbindClient(client._pToken);
	}

	Client clientEx = client;
	if (forceRoot)
	{
		clientEx._bounds = _vNodes[currentNode].GetBounds();
		clientEx._sphere = clientEx._bounds.GetSphere();
	}

	if (MustBind(currentNode, clientEx._bounds))
	{
		_vNodes[currentNode].BindClient(clientEx);
		return true;
	}
	else if (Node::HasChildren(currentNode, Utils::Cast32(_vNodes.size())))
	{
		VERUS_FOR(i, 8)
		{
			const int childIndex = Node::GetChildIndex(currentNode, i);
			if (BindClient(client, false, childIndex))
				return true;
		}
	}
	return false;
}

void Octree::UnbindClient(void* pToken)
{
	for (auto& node : _vNodes)
		node.UnbindClient(pToken);
}

void Octree::UpdateDynamicBounds(RcClient client)
{
	if (_vNodes.empty())
		return; // Octree is not ready.

	_vNodes[0].UpdateDynamicClient(client);
}

bool Octree::MustBind(int currentNode, RcBounds bounds) const
{
	if (Node::HasChildren(currentNode, Utils::Cast32(_vNodes.size())))
	{
		int count = 0;
		VERUS_FOR(i, 8)
		{
			const int childIndex = Node::GetChildIndex(currentNode, i);
			if (_vNodes[childIndex].GetBounds().IsOverlappingWith(bounds))
				count++;
			if (count > 1)
				return true;
		}
		return false;
	}
	return _vNodes[currentNode].GetBounds().IsOverlappingWith(bounds);
}

Continue Octree::TraverseVisible(RcFrustum frustum, PResult pResult, int currentNode, void* pUser)
{
	if (_vNodes.empty())
		return Continue::no;

	if (!currentNode)
	{
		_defaultResult = Result();
		if (!pResult)
			pResult = &_defaultResult;
		pResult->_testCount = 0;
		pResult->_passedTestCount = 0;
		pResult->_pLastFoundToken = nullptr;
		pResult->_depth = Scene::SceneManager::IsDrawingDepth(Scene::DrawDepth::automatic);
	}

	pResult->_testCount++;
	const float onePixel = Math::ComputeOnePixelDistance(
		_vNodes[currentNode].GetSphere().GetRadius());
	const bool notTooSmall = pResult->_depth || VMath::distSqr(
		frustum.GetEyePosition(), _vNodes[currentNode].GetSphere().GetCenter()) < onePixel * onePixel;

	if (notTooSmall &&
		Relation::outside != frustum.ContainsSphere(_vNodes[currentNode].GetSphere()) &&
		Relation::outside != frustum.ContainsAabb(_vNodes[currentNode].GetBounds()))
	{
		{
			RcNode node = _vNodes[currentNode];
			const int count = node.GetClientCount();
			VERUS_FOR(i, count)
			{
				RcClient client = node.GetClientAt(i);

				pResult->_testCount++;
				const float onePixel = Math::ComputeOnePixelDistance(
					client._sphere.GetRadius());
				const bool notTooSmall = pResult->_depth || VMath::distSqr(
					frustum.GetEyePosition(), client._sphere.GetCenter()) < onePixel * onePixel;

				if (notTooSmall &&
					Relation::outside != frustum.ContainsSphere(client._sphere) &&
					Relation::outside != frustum.ContainsAabb(client._bounds))
				{
					pResult->_pLastFoundToken = client._pToken;
					pResult->_passedTestCount++;
					if (Continue::no == _pDelegate->Octree_ProcessNode(pResult->_pLastFoundToken, pUser))
						return Continue::no;
				}
			}
		}

		if (Node::HasChildren(currentNode, Utils::Cast32(_vNodes.size())))
		{
			VERUS_FOR(i, 8)
			{
				const int childIndex = Node::GetChildIndex(currentNode, i);
				if (Continue::no == TraverseVisible(frustum, pResult, childIndex, pUser))
					return Continue::no;
			}
		}
	}
	return Continue::yes;
}

Continue Octree::TraverseVisible(RcPoint3 point, PResult pResult, int currentNode, void* pUser)
{
	if (_vNodes.empty())
		return Continue::no;

	if (!currentNode)
	{
		_defaultResult = Result();
		if (!pResult)
			pResult = &_defaultResult;
		pResult->_testCount = 0;
		pResult->_passedTestCount = 0;
		pResult->_pLastFoundToken = nullptr;
	}

	pResult->_testCount++;
	if (_vNodes[currentNode].GetBounds().IsInside(point))
	{
		{
			RcNode node = _vNodes[currentNode];
			const int count = node.GetClientCount();
			VERUS_FOR(i, count)
			{
				RcClient client = node.GetClientAt(i);
				pResult->_testCount++;
				if (client._bounds.IsInside(point))
				{
					pResult->_pLastFoundToken = client._pToken;
					pResult->_passedTestCount++;
					if (Continue::no == _pDelegate->Octree_ProcessNode(pResult->_pLastFoundToken, pUser))
						return Continue::no;
				}
			}
		}

		if (Node::HasChildren(currentNode, Utils::Cast32(_vNodes.size())))
		{
			BYTE remapped[8];
			RemapChildIndices(point, _vNodes[currentNode].GetBounds().GetCenter(), remapped);
			VERUS_FOR(i, 8)
			{
				const int childIndex = Node::GetChildIndex(currentNode, remapped[i]);
				if (Continue::no == TraverseVisible(point, pResult, childIndex, pUser))
					return Continue::no;
			}
		}
	}
	return Continue::yes;
}

void Octree::RemapChildIndices(RcPoint3 point, RcPoint3 center, BYTE childIndices[8])
{
	int i =
		(point.getX() >= center.getX() ? 1 : 0) |
		(point.getY() >= center.getY() ? 2 : 0) |
		(point.getZ() >= center.getZ() ? 4 : 0);
	childIndices[0] = i;
	childIndices[1] = i ^ 0x1;
	childIndices[2] = i ^ 0x2;
	childIndices[3] = i ^ 0x4;
	i = (~i) & 0x07;
	childIndices[4] = i ^ 0x1;
	childIndices[5] = i ^ 0x2;
	childIndices[6] = i ^ 0x4;
	childIndices[7] = i;
}
