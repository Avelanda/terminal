// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "ActionsTemplateSelectors.g.h"

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    struct ActionsTemplateSelectors : ActionsTemplateSelectorsT<ActionsTemplateSelectors>
    {
        ActionsTemplateSelectors() = default;

        Windows::UI::Xaml::DataTemplate SelectTemplateCore(const winrt::Windows::Foundation::IInspectable&, const winrt::Windows::UI::Xaml::DependencyObject&);
        Windows::UI::Xaml::DataTemplate SelectTemplateCore(const winrt::Windows::Foundation::IInspectable&);

        WINRT_PROPERTY(winrt::Windows::UI::Xaml::DataTemplate, SendInputTemplate);
        WINRT_PROPERTY(winrt::Windows::UI::Xaml::DataTemplate, CloseTabTemplate);
    };
}

namespace winrt::Microsoft::Terminal::Settings::Editor::factory_implementation
{
    BASIC_FACTORY(ActionsTemplateSelectors);
}
