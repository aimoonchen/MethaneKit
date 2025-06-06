/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/Base/BufferSet.h
Base implementation of the buffer-set interface.

******************************************************************************/

#pragma once

#include "Object.h"

#include <Methane/Graphics/RHI/IBufferSet.h>

namespace Methane::Graphics::Base
{

class Buffer;

class BufferSet
    : public Rhi::IBufferSet
    , public Object
{
public:
    BufferSet(Rhi::BufferType buffers_type, const Refs<Rhi::IBuffer>& buffer_refs);

    // Buffers interface
    Rhi::BufferType           GetType() const noexcept final  { return m_buffers_type; }
    Data::Size                GetCount() const noexcept final { return static_cast<Data::Size>(m_refs.size()); }
    const Refs<Rhi::IBuffer>& GetRefs() const noexcept final  { return m_refs; }
    std::string               GetNames() const noexcept final;
    Rhi::IBuffer&             operator[](Data::Index index) const final;

    [[nodiscard]] bool  SetState(Rhi::ResourceState state);
    [[nodiscard]] const Ptr<Rhi::IResourceBarriers>& GetSetupTransitionBarriers() const noexcept { return m_setup_transition_barriers; }
    [[nodiscard]] RawPtrSpan<Buffer> GetRawPtrs() const noexcept { return m_raw_ptrs; }

private:
    const Rhi::BufferType       m_buffers_type;
    Refs<Rhi::IBuffer>          m_refs;
    Ptrs<Rhi::IBuffer>          m_ptrs;
    RawPtrs<Buffer>             m_raw_ptrs;
    Ptr<Rhi::IResourceBarriers> m_setup_transition_barriers;
};

} // namespace Methane::Graphics::Base
