/*
** Sample Framework for deko3d Applications
**   CCmdMemRing.h: Memory provider class for dynamic command buffers
*/
#pragma once

#include "driver/display/deko3d/CMemPool.h"
#include "driver/display/deko3d/common.h"

template<unsigned NumSlices>
class CCmdVtxRing
{
    static_assert(NumSlices > 0, "Need a non-zero number of slices...");

    CMemPool::Handle m_mem;
    unsigned m_curSlice;
    uint32_t m_sliceSize;

  public:
    CCmdVtxRing() : m_mem {}, m_curSlice {}, m_sliceSize {}
    {}

    CCmdVtxRing(const CCmdVtxRing&) = delete;

    CCmdVtxRing& operator=(const CCmdVtxRing&) = delete;

    ~CCmdVtxRing()
    {
        m_mem.destroy();
    }

    bool allocate(CMemPool& pool, uint32_t size)
    {
        m_sliceSize = (size + DK_CMDMEM_ALIGNMENT - 1) & ~(DK_CMDMEM_ALIGNMENT - 1);
        m_mem       = pool.allocate(NumSlices * m_sliceSize, alignof(love::Vertex));

        return m_mem;
    }

    uint32_t getSize() const
    {
        return m_mem.getSize() / NumSlices;
    }

    /* Return the current slice data */
    std::pair<void*, DkGpuAddr> begin()
    {
        const auto offset = m_curSlice * m_sliceSize;
        return std::make_pair((void*)((char*)m_mem.getCpuAddr() + offset), m_mem.getGpuAddr() + offset);
    }

    /*
    ** Advance the current slice counter
    ** Wrap around when we reach the end
    */
    void end()
    {
        m_curSlice = (m_curSlice + 1) % NumSlices;
    }
};
