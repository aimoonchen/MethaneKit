/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/CommandKit.h
Methane CommandKit PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/ICommandKit.h>

namespace Methane::Graphics::Base
{
class CommandKit;
}

namespace Methane::Graphics::Rhi
{

class CommandQueue;
class RenderContext;
class RenderCommandList;
class ComputeCommandList;
class TransferCommandList;
class CommandListSet;

class CommandKit // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    using Interface = ICommandKit;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(CommandKit);
    META_PIMPL_METHODS_COMPARE_INLINE(CommandKit);

    META_PIMPL_API explicit CommandKit(const Ptr<ICommandKit>& interface_ptr);
    META_PIMPL_API explicit CommandKit(ICommandKit& interface_ref);
    META_PIMPL_API explicit CommandKit(const CommandQueue& command_queue);
    META_PIMPL_API CommandKit(const RenderContext& context, CommandListType command_lists_type);

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API ICommandKit& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<ICommandKit> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_PIMPL_API bool SetName(std::string_view name) const;
    META_PIMPL_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // ICommandKit interface methods
    [[nodiscard]] META_PIMPL_API const IContext&     GetContext() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API CommandQueue        GetQueue() const;
    [[nodiscard]] META_PIMPL_API CommandListType     GetListType() const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API bool                HasList(CommandListId cmd_list_id = 0U) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API bool                HasListWithState(CommandListState cmd_list_state, CommandListId cmd_list_id = 0U) const META_PIMPL_NOEXCEPT;
    [[nodiscard]] META_PIMPL_API RenderCommandList   GetRenderList(CommandListId cmd_list_id = 0U) const;
    [[nodiscard]] META_PIMPL_API RenderCommandList   GetRenderListForEncoding(CommandListId cmd_list_id = 0U, std::string_view debug_group_name = {}) const;
    [[nodiscard]] META_PIMPL_API ComputeCommandList  GetComputeList(CommandListId cmd_list_id = 0U) const;
    [[nodiscard]] META_PIMPL_API ComputeCommandList  GetComputeListForEncoding(CommandListId cmd_list_id = 0U, std::string_view debug_group_name = {}) const;
    [[nodiscard]] META_PIMPL_API TransferCommandList GetTransferList(CommandListId cmd_list_id = 0U) const;
    [[nodiscard]] META_PIMPL_API TransferCommandList GetTransferListForEncoding(CommandListId cmd_list_id = 0U, std::string_view debug_group_name = {}) const;
    [[nodiscard]] META_PIMPL_API CommandListSet      GetListSet(Rhi::CommandListIdSpan cmd_list_ids = g_default_command_list_ids,
                                                                Opt<Data::Index> frame_index_opt = {}) const;
    [[nodiscard]] META_PIMPL_API IFence&             GetFence(CommandListId fence_id = 0U) const;
    META_PIMPL_API void ExecuteListSet(Rhi::CommandListIdSpan cmd_list_ids = g_default_command_list_ids,
                                       Opt<Data::Index> frame_index_opt = {}) const;
    META_PIMPL_API void ExecuteListSetAndWaitForCompletion(Rhi::CommandListIdSpan cmd_list_ids = g_default_command_list_ids,
                                                           Opt<Data::Index> frame_index_opt = {}) const;

private:
    using Impl = Methane::Graphics::Base::CommandKit;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/CommandKit.cpp>

#endif // META_PIMPL_INLINE
