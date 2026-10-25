#pragma once

#include <cassert>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace Crumbs2D
{

static constexpr uint32_t INVALID_HANDLE = UINT32_MAX;
template <typename T> struct Column
{
    std::vector<T> Data;
    T DefaultValue;
};

template <typename C>
concept ColumnSet = requires(C& columns) { columns.ForEachColumn([](auto&) {}); };

struct Id
{
    uint32_t Handle;
    uint32_t Generation;
};

// A resolved row: one handle lookup, then direct column access. Constness follows TColumns.
// Invalidated by any Add or Remove on the same table.
template <typename TColumns> class RowView
{
public:
    RowView(TColumns& columns, uint32_t index) : m_Columns(columns), m_Index(index) {}

    template <typename T> auto& operator[](Column<T> std::remove_const_t<TColumns>::* column) const
    { return (m_Columns.*column).Data[m_Index]; }

    uint32_t Index() const { return m_Index; }

private:
    TColumns& m_Columns;
    uint32_t m_Index;
};

template <ColumnSet TColumns> class HandleTable
{
public:
    TColumns Columns;

    Id Add()
    {
        uint32_t handle;
        if (!m_FreeHandles.empty())
        {
            handle = m_FreeHandles.back();
            m_FreeHandles.pop_back();
        }
        else
        {
            handle = static_cast<uint32_t>(m_HandleToIndex.size());
            m_HandleToIndex.push_back(0);
            m_Generations.push_back(0);
        }

        const uint32_t index = m_Count++;
        Columns.ForEachColumn([](auto& column) { column.Data.push_back(column.DefaultValue); });
        m_IndexToHandle.push_back(handle);
        m_HandleToIndex[handle] = index;

        return {handle, m_Generations[handle]};
    }

    void Remove(Id id)
    {
        const uint32_t index = IndexOf(id);
        const uint32_t last = m_Count - 1;

        if (index != last)
        {
            Columns.ForEachColumn([&](auto& column) { column.Data[index] = column.Data[last]; });
            m_IndexToHandle[index] = m_IndexToHandle[last];
            m_HandleToIndex[m_IndexToHandle[index]] = index;
        }

        Columns.ForEachColumn([](auto& column) { column.Data.pop_back(); });
        m_IndexToHandle.pop_back();
        m_Count--;

        m_Generations[id.Handle]++;
        m_FreeHandles.push_back(id.Handle);
    }

    bool IsValid(Id id) const { return id.Handle < m_Generations.size() && m_Generations[id.Handle] == id.Generation; }

    uint32_t Count() const { return m_Count; }

    uint32_t IndexOf(Id id) const
    {
        assert(IsValid(id) && "Invalid handle: removed or recycled");
        return m_HandleToIndex[id.Handle];
    }

    RowView<TColumns> Row(Id id) { return {Columns, IndexOf(id)}; }
    RowView<const TColumns> Row(Id id) const { return {Columns, IndexOf(id)}; }
    RowView<TColumns> RowAt(uint32_t index) { return {Columns, index}; }
    RowView<const TColumns> RowAt(uint32_t index) const { return {Columns, index}; }

    template <typename T> T& Get(Id id, Column<T> TColumns::* column)
    {
        const uint32_t index = IndexOf(id);
        assert(index < (Columns.*column).Data.size() && "Desynchronized column");
        return (Columns.*column).Data[index];
    }

    template <typename T> const T& Get(Id id, Column<T> TColumns::* column) const
    {
        const uint32_t index = IndexOf(id);
        assert(index < (Columns.*column).Data.size() && "Desynchronized column");
        return (Columns.*column).Data[index];
    }

    Id IdAt(uint32_t index) const
    {
        assert(index < m_Count && "Index out of range");
        const uint32_t handle = m_IndexToHandle[index];
        return {handle, m_Generations[handle]};
    }

    Id IdOfHandle(uint32_t handle) const
    {
        assert(handle < m_Generations.size() && "Handle out of range");
        return {handle, m_Generations[handle]};
    }

private:
    uint32_t m_Count = 0;

    std::vector<uint32_t> m_FreeHandles;
    std::vector<uint32_t> m_HandleToIndex;
    std::vector<uint32_t> m_Generations;

    std::vector<uint32_t> m_IndexToHandle;
};
} // namespace Crumbs2D