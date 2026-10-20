#pragma once
#include "AABB.h"
#include <cstdint>
#include <vector>

namespace Crumbs2D
{
inline constexpr uint32_t NULL_NODE = UINT32_MAX;
inline constexpr uint16_t FREE_NODE_HEIGHT = UINT16_MAX;

struct NodeChildren
{
    uint32_t Child1;
    uint32_t Child2;
};

struct TreeNode
{
    AABB Bound{};
    uint32_t Parent = NULL_NODE;

    union
    {
        NodeChildren Children; // nœud interne
        uint32_t UserId;       // feuille
    };

    uint16_t Height = 0;
    bool IsLeaf = true;

    TreeNode() : Children{NULL_NODE, NULL_NODE} {}
};

class DynamicTree
{
private:
    std::vector<TreeNode> m_NodePool;
    std::vector<uint32_t> m_FreeNodeIndices;
    uint32_t m_RootIndex = NULL_NODE;
    float m_Margin;

    std::vector<uint32_t> m_NodeStack;

private:
    uint32_t AllocateNode();
    void FreeNode(uint32_t index);
    void InsertLeaf(uint32_t index);
    void RemoveLeaf(uint32_t index);
    void RefitFromNode(uint32_t index);
    void UpdateNode(uint32_t index);

    void Rotate(uint32_t index);
    float EstimatedInsertionCost(uint32_t childIndex, const AABB& leafBound) const;
    void InsertAsSibling(uint32_t sibling, uint32_t leaf);

public:
    explicit DynamicTree(float margin = 0.1f) : m_Margin(margin) {}

    bool Validate() const;

    // Ajoute un objet et renvoie l'index de sa feuille (stable pendant toute sa vie).
    uint32_t CreateProxy(const AABB& bound, uint32_t userId);
    void RemoveProxy(uint32_t leaf);
    // Renvoie vrai si la feuille a dû être réinsérée (sortie de sa borne agrandie).
    bool MoveProxy(const AABB& newBound, uint32_t leaf);

    uint32_t GetUserId(uint32_t leaf) const;
    const AABB& GetFatBound(uint32_t leaf) const;

    template <typename F> void Query(const AABB& query, F&& callback);

    template <typename F> void ForEachNode(F&& f) const
    {
        for (uint32_t i = 0; i < m_NodePool.size(); i++)
        {
            const TreeNode& node = m_NodePool[i];
            if (node.Height != FREE_NODE_HEIGHT)
                f(node.Bound, node.Height, node.IsLeaf);
        }
    }
};

template <typename F> inline void DynamicTree::Query(const AABB& query, F&& callback)
{
    m_NodeStack.clear();
    if (m_RootIndex == NULL_NODE)
        return;
    m_NodeStack.push_back(m_RootIndex);
    while (!m_NodeStack.empty())
    {
        uint32_t currentID = m_NodeStack.back();
        m_NodeStack.pop_back();
        const TreeNode& currentNode = m_NodePool[currentID];
        if (AABB::TestOverlap(currentNode.Bound, query))
        {
            if (currentNode.IsLeaf)
            {
                callback(currentNode.UserId);
            }
            else
            {
                m_NodeStack.push_back(currentNode.Children.Child1);
                m_NodeStack.push_back(currentNode.Children.Child2);
            }
        }
    }
}

} // namespace Crumbs2D