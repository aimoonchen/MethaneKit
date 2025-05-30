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

FILE: Methane/Platform/AppController.cpp
Base application controller providing commands like app close and help.

******************************************************************************/

#include <Methane/Platform/AppController.h>
#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Platform
{

AppController::AppController(AppBase& application, const std::string& application_help, const ActionByKeyboardState& action_by_keyboard_state)
    : Controller(application_help)
    , Input::Keyboard::ActionControllerBase<AppAction>(action_by_keyboard_state, {})
    , m_application(application)
{ }

void AppController::OnKeyboardChanged(Input::Keyboard::Key key, Input::Keyboard::KeyState key_state, const Input::Keyboard::StateChange& state_change)
{
    META_FUNCTION_TASK();
    Input::Keyboard::ActionControllerBase<AppAction>::OnKeyboardChanged(key, key_state, state_change);
}

void AppController::OnKeyboardStateAction(AppAction action)
{
    META_FUNCTION_TASK();
    switch(action)
    {
    using enum AppAction;
    case ShowControlsHelp:    m_application.ShowControlsHelp(); break;
    case ShowCommandLineHelp: m_application.ShowCommandLineHelp(); break;
    case ShowParameters:      m_application.ShowParameters(); break;
    case SwitchFullScreen:    m_application.SetFullScreen(!m_application.GetPlatformAppSettings().is_full_screen); break;
    case CloseApp:            m_application.Close(); break;
    default:                             META_UNEXPECTED(action);
    }
}

std::string AppController::GetKeyboardActionName(AppAction action) const
{
    META_FUNCTION_TASK();
    switch (action)
    {
    using enum AppAction;
    case None:                return "none";
    case ShowControlsHelp:    return "show application controls help";
    case ShowCommandLineHelp: return "show application command-line help";
    case ShowParameters:      return "show application parameters";
    case SwitchFullScreen:    return "switch full-screen mode";
    case CloseApp:            return "close the application";
    default:                             META_UNEXPECTED_RETURN(action, "");
    }
}

Input::IHelpProvider::HelpLines AppController::GetHelp() const
{
    META_FUNCTION_TASK();
    return GetKeyboardHelp();
}

} // namespace Methane::Platform
