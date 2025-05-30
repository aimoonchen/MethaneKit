/******************************************************************************

Copyright 2024 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/RootConstant.h
Methane root constant value, used to set program argument binding value directly

******************************************************************************/

#pragma once

#include <Methane/Data/Chunk.hpp>

namespace Methane::Graphics::Rhi
{

class RootConstant
    : public Data::Chunk
{
public:
    RootConstant() = default;

    using Chunk::Chunk;

    explicit RootConstant(Chunk&& chunk)
        : Data::Chunk(std::move(chunk))
    { }

    template<typename T>
    const T& GetValue() const
    {
        META_CHECK_EQUAL_DESCR(sizeof(T), Data::Chunk::GetDataSize(),
                               "size of value type does not match with root constant data size");
        return reinterpret_cast<const T&>(*Data::Chunk::GetDataPtr()); // NOSONAR
    }

    static RootConstant StoreFrom(const Chunk& other)
    {
        return RootConstant(Data::Chunk::StoreFrom(other));
    }
};

} // namespace Methane::Graphics::Rhi
