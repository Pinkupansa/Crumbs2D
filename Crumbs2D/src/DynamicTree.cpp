#include "DynamicTree.h"
#include <algorithm>
namespace Crumbs2D
{
uint32_t DynamicTree::AllocateNode()
{
    uint32_t index;
    if (m_FreeNodeIndices.empty())
    {
        index = static_cast<uint32_t>(m_NodePool.size());
        m_NodePool.emplace_back();
    }
    else
    {
        index = m_FreeNodeIndices.back();
        m_FreeNodeIndices.pop_back();
        m_NodePool[index] = TreeNode{};
    }
    return index;
}
void DynamicTree::FreeNode(uint32_t index)
{
    m_FreeNodeIndices.push_back(index);
    m_NodePool[index].Height = FREE_NODE_HEIGHT;
}
void DynamicTree::RefitFromNode(uint32_t index)
{
    uint32_t current = index;
    while (current != NULL_NODE)
    {
        Rotate(current);
        UpdateNode(current);
        current = m_NodePool[current].Parent;
    }
}

// Recalcule la borne et la hauteur d'un nœud interne à partir de ses enfants.
void DynamicTree::UpdateNode(uint32_t index)
{
    TreeNode& node = m_NodePool[index];
    const TreeNode& child1 = m_NodePool[node.Children.Child1];
    const TreeNode& child2 = m_NodePool[node.Children.Child2];
    node.Bound = AABB::Combine(child1.Bound, child2.Bound);
    node.Height = static_cast<uint16_t>(std::max(child1.Height, child2.Height) + 1);
}

// Échange un enfant de A avec un petit-enfant de l'autre côté,
// si cela réduit le périmètre du nœud intermédiaire modifié.
void DynamicTree::Rotate(uint32_t iA)
{
    const TreeNode& A = m_NodePool[iA];
    if (A.IsLeaf || A.Height < 2)
        return;

    const uint32_t iB = A.Children.Child1;
    const uint32_t iC = A.Children.Child2;
    const TreeNode& B = m_NodePool[iB];
    const TreeNode& C = m_NodePool[iC];

    enum class Swap { None, BF, BG, CD, CE };
    Swap best = Swap::None;
    float bestDelta = 0.0f;

    if (!C.IsLeaf)
    {
        const AABB& F = m_NodePool[C.Children.Child1].Bound;
        const AABB& G = m_NodePool[C.Children.Child2].Bound;
        const float costC = AABB::Cost(C.Bound);
        const float deltaBF = AABB::Cost(AABB::Combine(B.Bound, G)) - costC; // C contiendrait B et G
        const float deltaBG = AABB::Cost(AABB::Combine(B.Bound, F)) - costC; // C contiendrait B et F
        if (deltaBF < bestDelta)
        {
            bestDelta = deltaBF;
            best = Swap::BF;
        }
        if (deltaBG < bestDelta)
        {
            bestDelta = deltaBG;
            best = Swap::BG;
        }
    }
    if (!B.IsLeaf)
    {
        const AABB& D = m_NodePool[B.Children.Child1].Bound;
        const AABB& E = m_NodePool[B.Children.Child2].Bound;
        const float costB = AABB::Cost(B.Bound);
        const float deltaCD = AABB::Cost(AABB::Combine(C.Bound, E)) - costB; // B contiendrait C et E
        const float deltaCE = AABB::Cost(AABB::Combine(C.Bound, D)) - costB; // B contiendrait C et D
        if (deltaCD < bestDelta)
        {
            bestDelta = deltaCD;
            best = Swap::CD;
        }
        if (deltaCE < bestDelta)
        {
            bestDelta = deltaCE;
            best = Swap::CE;
        }
    }

    // Échange l'enfant `child` de A avec le petit-enfant `grandChild`, rangé sous `middle`.
    auto swapWithGrandChild = [&](uint32_t child, uint32_t middle, uint32_t grandChild) {
        NodeChildren& top = m_NodePool[iA].Children;
        if (top.Child1 == child)
            top.Child1 = grandChild;
        else
            top.Child2 = grandChild;

        NodeChildren& mid = m_NodePool[middle].Children;
        if (mid.Child1 == grandChild)
            mid.Child1 = child;
        else
            mid.Child2 = child;

        m_NodePool[grandChild].Parent = iA;
        m_NodePool[child].Parent = middle;
        UpdateNode(middle);
    };

    switch (best)
    {
        case Swap::None:
            break;
        case Swap::BF:
            swapWithGrandChild(iB, iC, C.Children.Child1);
            break;
        case Swap::BG:
            swapWithGrandChild(iB, iC, C.Children.Child2);
            break;
        case Swap::CD:
            swapWithGrandChild(iC, iB, B.Children.Child1);
            break;
        case Swap::CE:
            swapWithGrandChild(iC, iB, B.Children.Child2);
            break;
    }
}
float DynamicTree::EstimatedInsertionCost(uint32_t childIndex, const AABB& leafBound) const
{
    const TreeNode& child = m_NodePool[childIndex];
    const float combinedCost = AABB::Cost(AABB::Combine(child.Bound, leafBound));

    // Feuille : on créerait un nouveau parent qui englobe les deux.
    if (child.IsLeaf)
        return combinedCost;

    // Nœud interne : au minimum, il doit grossir pour contenir la nouvelle feuille.
    return combinedCost - AABB::Cost(child.Bound);
}
void DynamicTree::InsertLeaf(uint32_t index)
{
    if (m_RootIndex == NULL_NODE)
    {
        m_RootIndex = index;
        return;
    }
    uint32_t current = m_RootIndex;
    const AABB& newBound = m_NodePool[index].Bound;
    while (!m_NodePool[current].IsLeaf)
    {
        const TreeNode& currentNode = m_NodePool[current];

        // STOPPING COST
        float costUnionBox = AABB::Cost(AABB::Combine(currentNode.Bound, m_NodePool[index].Bound));

        // INSERTION COST
        float deltaUnion = costUnionBox - AABB::Cost(currentNode.Bound);

        float totalCostInsert1 = deltaUnion + EstimatedInsertionCost(currentNode.Children.Child1, newBound);

        float totalCostInsert2 = deltaUnion + EstimatedInsertionCost(currentNode.Children.Child2, newBound);

        bool insertInFirst = totalCostInsert1 <= totalCostInsert2;
        float insertCost = insertInFirst ? totalCostInsert1 : totalCostInsert2;

        if (insertCost <= costUnionBox)
        {
            // insert in child
            current = insertInFirst ? currentNode.Children.Child1 : currentNode.Children.Child2;
        }
        else
        {
            // stop here
            break;
        }
    }
    InsertAsSibling(current, index);
}
void DynamicTree::InsertAsSibling(uint32_t sibling, uint32_t leaf)
{
    const uint32_t oldParent = m_NodePool[sibling].Parent;

    // AllocateNode peut réallouer le pool : aucune référence ne doit survivre à cet appel.
    const uint32_t newParent = AllocateNode();

    TreeNode& parentNode = m_NodePool[newParent];
    parentNode.IsLeaf = false;
    parentNode.Parent = oldParent;
    parentNode.Children = {sibling, leaf};
    parentNode.Bound = AABB::Combine(m_NodePool[sibling].Bound, m_NodePool[leaf].Bound);
    parentNode.Height = static_cast<uint16_t>(m_NodePool[sibling].Height + 1);

    m_NodePool[sibling].Parent = newParent;
    m_NodePool[leaf].Parent = newParent;

    // Le nouveau parent prend la place du frère auprès de l'ancien parent.
    if (oldParent == NULL_NODE)
    {
        m_RootIndex = newParent;
    }
    else
    {
        NodeChildren& children = m_NodePool[oldParent].Children;
        if (children.Child1 == sibling)
            children.Child1 = newParent;
        else
            children.Child2 = newParent;
    }

    // Les ancêtres doivent grossir pour contenir la nouvelle feuille.
    RefitFromNode(oldParent);
}

void DynamicTree::RemoveLeaf(uint32_t index)
{
    const uint32_t parentIndex = m_NodePool[index].Parent;
    m_NodePool[index].Parent = NULL_NODE;

    if (parentIndex == NULL_NODE)
    {
        m_RootIndex = NULL_NODE;
        return;
    }

    const NodeChildren parentChildren = m_NodePool[parentIndex].Children;
    const uint32_t siblingIndex = parentChildren.Child1 == index ? parentChildren.Child2 : parentChildren.Child1;
    const uint32_t grandParentIndex = m_NodePool[parentIndex].Parent;

    m_NodePool[siblingIndex].Parent = grandParentIndex;

    if (grandParentIndex == NULL_NODE)
    {
        m_RootIndex = siblingIndex;
    }
    else
    {
        NodeChildren& grandParentChildren = m_NodePool[grandParentIndex].Children;
        if (grandParentChildren.Child1 == parentIndex)
            grandParentChildren.Child1 = siblingIndex;
        else
            grandParentChildren.Child2 = siblingIndex;
    }

    FreeNode(parentIndex);
    RefitFromNode(grandParentIndex);
}

bool DynamicTree::Validate() const
{
    const uint32_t poolSize = static_cast<uint32_t>(m_NodePool.size());

    // Free nodes should be marked free
    for (uint32_t index : m_FreeNodeIndices)
        if (index >= poolSize || m_NodePool[index].Height != FREE_NODE_HEIGHT)
            return false;

    // All nodes should be free when the tree does not have a root
    if (m_RootIndex == NULL_NODE)
        return m_FreeNodeIndices.size() == poolSize;

    // The root should not have a parent
    if (m_RootIndex >= poolSize || m_NodePool[m_RootIndex].Parent != NULL_NODE)
        return false;

    std::vector<uint32_t> stack = {m_RootIndex};
    uint32_t reached = 0;

    while (!stack.empty())
    {
        const uint32_t index = stack.back();
        stack.pop_back();

        // there should not be more reached nodes than the pool size
        if (++reached > poolSize)
            return false;

        const TreeNode& node = m_NodePool[index];
        // A reached node can't be free
        if (node.Height == FREE_NODE_HEIGHT)
            return false;

        // Leaves have height 0
        if (node.IsLeaf)
        {
            if (node.Height != 0)
                return false;
            continue;
        }

        const uint32_t child1 = node.Children.Child1;
        const uint32_t child2 = node.Children.Child2;

        if (child1 >= poolSize || child2 >= poolSize)
            return false;

        // A node's children's parent should be the node itself
        if (m_NodePool[child1].Parent != index || m_NodePool[child2].Parent != index)
            return false;

        // Parents contain their children
        if (!AABB::Contains(node.Bound, m_NodePool[child1].Bound) ||
            !AABB::Contains(node.Bound, m_NodePool[child2].Bound))
            return false;

        // The height should satisfy this equality
        if (node.Height != 1 + std::max(m_NodePool[child1].Height, m_NodePool[child2].Height))
            return false;

        stack.push_back(child1);
        stack.push_back(child2);
    }

    return reached + m_FreeNodeIndices.size() == poolSize;
}

uint32_t DynamicTree::CreateProxy(const AABB& bound, uint32_t userID)
{
    const uint32_t index = AllocateNode();
    m_NodePool[index].Bound = AABB::Fatten(bound, 0.1f);
    m_NodePool[index].UserId = userID;
    m_NodePool[index].IsLeaf = true;
    InsertLeaf(index);
    return index;
}

void DynamicTree::RemoveProxy(uint32_t index)
{
    RemoveLeaf(index);
    FreeNode(index);
}

bool DynamicTree::MoveProxy(const AABB& newBound, uint32_t index)
{
    if (!AABB::Contains(m_NodePool[index].Bound, newBound))
    {
        RemoveLeaf(index);
        m_NodePool[index].Bound = AABB::Fatten(newBound, 0.1f);
        InsertLeaf(index);
        return true;
    }
    return false;
}
uint32_t DynamicTree::GetUserId(uint32_t leaf) const
{
    assert(m_NodePool[leaf].IsLeaf);
    return m_NodePool[leaf].UserId;
}

const AABB& DynamicTree::GetFatBound(uint32_t leaf) const
{
    assert(m_NodePool[leaf].IsLeaf);
    return m_NodePool[leaf].Bound;
}

} // namespace Crumbs2D