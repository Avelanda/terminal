// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "NewTabMenuViewModel.h"
#include <LibraryResources.h>

#include "NewTabMenuViewModel.g.cpp"
#include "NewTabMenuEntryViewModel.g.cpp"
#include "ProfileEntryViewModel.g.cpp"
#include "SeparatorEntryViewModel.g.cpp"
#include "FolderEntryViewModel.g.cpp"
#include "MatchProfilesEntryViewModel.g.cpp"
#include "RemainingProfilesEntryViewModel.g.cpp"

using namespace winrt::Windows::UI::Xaml::Navigation;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Microsoft::Terminal::Settings::Model;
using namespace winrt::Windows::UI::Xaml::Data;

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    static IObservableVector<Editor::NewTabMenuEntryViewModel> _ConvertToViewModelEntries(IVector<Model::NewTabMenuEntry> settingsModelEntries)
    {
        auto result = single_threaded_observable_vector<Editor::NewTabMenuEntryViewModel>();
        for (const auto& entry : settingsModelEntries)
        {
            switch (entry.Type())
            {
            case NewTabMenuEntryType::Profile:
            {
                // If the Profile isn't set, this is an invalid entry. Skip it.
                if (const auto& profileEntry = entry.as<Model::ProfileEntry>(); profileEntry.Profile())
                {
                    const auto profileEntryVM = make<ProfileEntryViewModel>(profileEntry);
                    result.Append(profileEntryVM);
                }
                break;
            }
            case NewTabMenuEntryType::Separator:
            {
                if (const auto& separatorEntry = entry.as<Model::SeparatorEntry>())
                {
                    const auto separatorEntryVM = make<SeparatorEntryViewModel>(separatorEntry);
                    result.Append(separatorEntryVM);
                }
                break;
            }
            case NewTabMenuEntryType::Folder:
            {
                if (const auto& folderEntry = entry.as<Model::FolderEntry>())
                {
                    // The ctor will convert the children of the folder to view models
                    const auto folderEntryVM = make<FolderEntryViewModel>(folderEntry);
                    result.Append(folderEntryVM);
                }
                break;
            }
            case NewTabMenuEntryType::MatchProfiles:
            {
                if (const auto& matchProfilesEntry = entry.as<Model::MatchProfilesEntry>())
                {
                    const auto matchProfilesEntryVM = make<MatchProfilesEntryViewModel>(matchProfilesEntry);
                    result.Append(matchProfilesEntryVM);
                }
                break;
            }
            case NewTabMenuEntryType::RemainingProfiles:
            {
                if (const auto& remainingProfilesEntry = entry.as<Model::RemainingProfilesEntry>())
                {
                    const auto remainingProfilesEntryVM = make<RemainingProfilesEntryViewModel>(remainingProfilesEntry);
                    result.Append(remainingProfilesEntryVM);
                }
                break;
            }
            case NewTabMenuEntryType::Invalid:
            default:
                break;
            }
        }
        return result;
    }

    bool NewTabMenuViewModel::IsRemainingProfilesEntryMissing(const IObservableVector<Editor::NewTabMenuEntryViewModel>& entries)
    {
        for (const auto& entry : entries)
        {
            switch (entry.Type())
            {
            case NewTabMenuEntryType::RemainingProfiles:
            {
                return false;
            }
            case NewTabMenuEntryType::Folder:
            {
                if (!IsRemainingProfilesEntryMissing(entry.as<Editor::FolderEntryViewModel>().Entries()))
                {
                    return false;
                }
            }
            default:
                break;
            }
        }
        return true;
    };

    bool NewTabMenuViewModel::IsFolderView() const noexcept
    {
        return _CurrentFolderEntry != nullptr;
    }

    NewTabMenuViewModel::NewTabMenuViewModel(Model::CascadiaSettings settings)
    {
        UpdateSettings(settings);

        _UpdateEntries();

        // Add a property changed handler to our own property changed event.
        // This propagates changes from the settings model to anybody listening to our
        // unique view model members.
        PropertyChanged([this](auto&&, const PropertyChangedEventArgs& args) {
            const auto viewModelProperty{ args.PropertyName() };
            if (viewModelProperty == L"AvailableProfiles")
            {
                _NotifyChanges(L"SelectedProfile");
            }
            else if (viewModelProperty == L"CurrentFolderEntry")
            {
                _NotifyChanges(L"IsFolderView");
                _UpdateEntries();
            }
        });
    }

    void NewTabMenuViewModel::UpdateSettings(Model::CascadiaSettings settings)
    {
        _Settings = settings;
        _NotifyChanges(L"AvailableProfiles");

        SelectedProfile(AvailableProfiles().GetAt(0));
    }

    void NewTabMenuViewModel::_UpdateEntries()
    {
        // TODO CARLOS: shorthand
        //Entries(_CurrentFolderEntry ? _CurrentFolderEntry.Entries() : _ConvertToViewModelEntries(_Settings.GlobalSettings().NewTabMenu()));
        _entriesChangedRevoker.revoke();
        if (_CurrentFolderEntry)
        {
            Entries(_CurrentFolderEntry.Entries());
        }
        else
        {
            Entries(_ConvertToViewModelEntries(_Settings.GlobalSettings().NewTabMenu()));
        }

        // TODO CARLOS: I have the revoker here, not sure when/how to use it though, but I suspect we need it
        _entriesChangedRevoker = _Entries.VectorChanged(winrt::auto_revoke, [this](auto&&, const IVectorChangedEventArgs& args) {
            switch (args.CollectionChange())
            {
            case CollectionChange::Reset:
            {
                // fully replace settings model with _Entries
                for (const auto& entry : _Entries)
                {
                    auto modelEntries = single_threaded_vector<Model::NewTabMenuEntry>();
                    modelEntries.Append(NewTabMenuEntryViewModel::GetModel(entry));

                    if (_CurrentFolderEntry)
                    {
                        FolderEntry modelCurrentFolder = NewTabMenuEntryViewModel::GetModel(_CurrentFolderEntry).as<FolderEntry>();
                        modelCurrentFolder.RawEntries(modelEntries);
                    }
                    else
                    {
                        _Settings.GlobalSettings().NewTabMenu(modelEntries);
                    }
                }
                return;
            }
            case CollectionChange::ItemInserted:
            {
                const auto& insertedEntryVM = _Entries.GetAt(args.Index());
                const auto& insertedEntry = NewTabMenuEntryViewModel::GetModel(insertedEntryVM);

                if (_CurrentFolderEntry)
                {
                    FolderEntry modelCurrentFolder = NewTabMenuEntryViewModel::GetModel(_CurrentFolderEntry).as<FolderEntry>();
                    modelCurrentFolder.RawEntries().InsertAt(args.Index(), insertedEntry);
                }
                else
                {
                    auto newTabMenu = _Settings.GlobalSettings().NewTabMenu();
                    newTabMenu.InsertAt(args.Index(), insertedEntry);
                }
                return;
            }
            case CollectionChange::ItemRemoved:
            {
                if (_CurrentFolderEntry)
                {
                    FolderEntry modelCurrentFolder = NewTabMenuEntryViewModel::GetModel(_CurrentFolderEntry).as<FolderEntry>();
                    modelCurrentFolder.RawEntries().RemoveAt(args.Index());
                }
                else
                {
                    auto newTabMenu = _Settings.GlobalSettings().NewTabMenu();
                    newTabMenu.RemoveAt(args.Index());
                }
                return;
            }
            case CollectionChange::ItemChanged:
            {
                if (_CurrentFolderEntry)
                {
                    FolderEntry modelCurrentFolder = NewTabMenuEntryViewModel::GetModel(_CurrentFolderEntry).as<FolderEntry>();
                    const auto modifiedEntry = _Entries.GetAt(args.Index());
                    modelCurrentFolder.Entries().SetAt(args.Index(), NewTabMenuEntryViewModel::GetModel(modifiedEntry));
                }
                else
                {
                    auto newTabMenu = _Settings.GlobalSettings().NewTabMenu();
                    const auto modifiedEntry = _Entries.GetAt(args.Index());
                    newTabMenu.SetAt(args.Index(), NewTabMenuEntryViewModel::GetModel(modifiedEntry));
                }
                return;
            }
            }
        });
    }

    void NewTabMenuViewModel::RequestReorderEntry(Editor::NewTabMenuEntryViewModel vm, bool goingUp)
    {
        uint32_t idx;
        if (_Entries.IndexOf(vm, idx))
        {
            if (goingUp && idx > 0)
            {
                _Entries.RemoveAt(idx);
                _Entries.InsertAt(idx - 1, vm);
            }
            else if (!goingUp && idx < _Entries.Size() - 1)
            {
                _Entries.RemoveAt(idx);
                _Entries.InsertAt(idx + 1, vm);
            }
        }
    }

    void NewTabMenuViewModel::RequestDeleteEntry(Editor::NewTabMenuEntryViewModel vm)
    {
        uint32_t idx;
        if (_Entries.IndexOf(vm, idx))
        {
            _Entries.RemoveAt(idx);
        }
    }

    void NewTabMenuViewModel::RequestAddSelectedProfileEntry()
    {
        if (_SelectedProfile)
        {
            Model::ProfileEntry profileEntry;
            profileEntry.Profile(_SelectedProfile);

            _Entries.Append(make<ProfileEntryViewModel>(profileEntry));
        }
        _PrintAll();
    }

    void NewTabMenuViewModel::RequestAddSeparatorEntry()
    {
        Model::SeparatorEntry separatorEntry;

        _Entries.Append(make<SeparatorEntryViewModel>(separatorEntry));

        _PrintAll();
    }

    void NewTabMenuViewModel::RequestAddFolderEntry()
    {
        Model::FolderEntry folderEntry;
        folderEntry.Name(_FolderName);

        _Entries.Append(make<FolderEntryViewModel>(folderEntry));

        // Clear the field after adding the entry
        FolderName({});

        _PrintAll();
    }

    void NewTabMenuViewModel::RequestAddProfileMatcherEntry()
    {
        Model::MatchProfilesEntry matchProfilesEntry;
        matchProfilesEntry.Name(_ProfileMatcherName);
        matchProfilesEntry.Source(_ProfileMatcherSource);
        matchProfilesEntry.Commandline(_ProfileMatcherCommandline);

        _Entries.Append(make<MatchProfilesEntryViewModel>(matchProfilesEntry));

        // Clear the fields after adding the entry
        ProfileMatcherName({});
        ProfileMatcherSource({});
        ProfileMatcherCommandline({});

        _PrintAll();
    }

    void NewTabMenuViewModel::RequestAddRemainingProfilesEntry()
    {
        Model::RemainingProfilesEntry remainingProfilesEntry;
        _Entries.Append(make<RemainingProfilesEntryViewModel>(remainingProfilesEntry));

        _PrintAll();
    }

    void NewTabMenuViewModel::_PrintAll()
    {
#ifdef _DEBUG
        OutputDebugString(L"---Model:---\n");
        _PrintModel(_Settings.GlobalSettings().NewTabMenu());
        OutputDebugString(L"\n");
        OutputDebugString(L"---VM (Current Layer):---\n");
        _PrintVM(Entries());
        OutputDebugString(L"\n");
#endif
    }

#ifdef _DEBUG
    void NewTabMenuViewModel::_PrintModel(Windows::Foundation::Collections::IVector<Model::NewTabMenuEntry> list, std::wstring prefix)
    {
        for (auto&& e : list)
        {
            _PrintModel(e, prefix);
        }
    }

    void NewTabMenuViewModel::_PrintModel(const Model::NewTabMenuEntry& e, std::wstring prefix)
    {
        switch (e.Type())
        {
        case NewTabMenuEntryType::Profile:
        {
            const auto& pe = e.as<Model::ProfileEntry>();
            OutputDebugString(fmt::format(L"{}Profile: {}\n", prefix, pe.Profile().Name()).c_str());
            break;
        }
        case NewTabMenuEntryType::Separator:
        {
            OutputDebugString(fmt::format(L"{}Separator\n", prefix).c_str());
            break;
        }
        case NewTabMenuEntryType::Folder:
        {
            const auto& fe = e.as<Model::FolderEntry>();
            OutputDebugString(fmt::format(L"{}Folder: {}\n", prefix, fe.Name()).c_str());
            _PrintModel(fe.RawEntries(), prefix + L"  ");
            break;
        }
        case NewTabMenuEntryType::MatchProfiles:
        {
            const auto& mpe = e.as<Model::MatchProfilesEntry>();
            OutputDebugString(fmt::format(L"{}MatchProfiles: {}\n", prefix, mpe.Name()).c_str());
            break;
        }
        case NewTabMenuEntryType::RemainingProfiles:
        {
            OutputDebugString(fmt::format(L"{}RemainingProfiles\n", prefix).c_str());
            break;
        }
        default:
            break;
        }
    }

    void NewTabMenuViewModel::_PrintVM(Windows::Foundation::Collections::IVector<Editor::NewTabMenuEntryViewModel> list, std::wstring prefix)
    {
        for (auto&& e : list)
        {
            _PrintVM(e);
        }
    }

    void NewTabMenuViewModel::_PrintVM(const Editor::NewTabMenuEntryViewModel& e, std::wstring prefix)
    {
        switch (e.Type())
        {
        case NewTabMenuEntryType::Profile:
        {
            const auto& pe = e.as<Editor::ProfileEntryViewModel>();
            OutputDebugString(fmt::format(L"{}Profile: {}\n", prefix, pe.ProfileEntry().Profile().Name()).c_str());
            break;
        }
        case NewTabMenuEntryType::Separator:
        {
            OutputDebugString(fmt::format(L"{}Separator\n", prefix).c_str());
            break;
        }
        case NewTabMenuEntryType::Folder:
        {
            const auto& fe = e.as<Editor::FolderEntryViewModel>();
            OutputDebugString(fmt::format(L"{}Folder: {}\n", prefix, fe.Name()).c_str());
            _PrintVM(fe.Entries(), prefix + L"  ");
            break;
        }
        case NewTabMenuEntryType::MatchProfiles:
        {
            const auto& mpe = e.as<Editor::MatchProfilesEntryViewModel>();
            OutputDebugString(fmt::format(L"{}MatchProfiles: {}\n", prefix, mpe.DisplayText()).c_str());
            break;
        }
        case NewTabMenuEntryType::RemainingProfiles:
        {
            OutputDebugString(fmt::format(L"{}RemainingProfiles\n", prefix).c_str());
            break;
        }
        default:
            break;
        }
    }
#endif

    NewTabMenuEntryViewModel::NewTabMenuEntryViewModel(const NewTabMenuEntryType type) noexcept :
        _Type{ type }
    {
    }

    Model::NewTabMenuEntry NewTabMenuEntryViewModel::GetModel(const Editor::NewTabMenuEntryViewModel& viewModel)
    {
        switch (viewModel.Type())
        {
        case NewTabMenuEntryType::Profile:
        {
            const auto& projVM = viewModel.as<Editor::ProfileEntryViewModel>();
            return get_self<ProfileEntryViewModel>(projVM)->ProfileEntry();
        }
        case NewTabMenuEntryType::Separator:
        {
            const auto& projVM = viewModel.as<Editor::SeparatorEntryViewModel>();
            return get_self<SeparatorEntryViewModel>(projVM)->SeparatorEntry();
        }
        case NewTabMenuEntryType::Folder:
        {
            const auto& projVM = viewModel.as<Editor::FolderEntryViewModel>();
            return get_self<FolderEntryViewModel>(projVM)->FolderEntry();
        }
        case NewTabMenuEntryType::MatchProfiles:
        {
            const auto& projVM = viewModel.as<Editor::MatchProfilesEntryViewModel>();
            return get_self<MatchProfilesEntryViewModel>(projVM)->MatchProfilesEntry();
        }
        case NewTabMenuEntryType::RemainingProfiles:
        {
            const auto& projVM = viewModel.as<Editor::RemainingProfilesEntryViewModel>();
            return get_self<RemainingProfilesEntryViewModel>(projVM)->RemainingProfilesEntry();
        }
        case NewTabMenuEntryType::Invalid:
        default:
            return nullptr;
        }
    }

    ProfileEntryViewModel::ProfileEntryViewModel(Model::ProfileEntry profileEntry) :
        ProfileEntryViewModelT<ProfileEntryViewModel, NewTabMenuEntryViewModel>(Model::NewTabMenuEntryType::Profile),
        _ProfileEntry{ profileEntry }
    {
    }

    SeparatorEntryViewModel::SeparatorEntryViewModel(Model::SeparatorEntry separatorEntry) :
        SeparatorEntryViewModelT<SeparatorEntryViewModel, NewTabMenuEntryViewModel>(Model::NewTabMenuEntryType::Separator),
        _SeparatorEntry{ separatorEntry }
    {
    }

    FolderEntryViewModel::FolderEntryViewModel(Model::FolderEntry folderEntry) :
        FolderEntryViewModelT<FolderEntryViewModel, NewTabMenuEntryViewModel>(Model::NewTabMenuEntryType::Folder),
        _FolderEntry{ folderEntry }
    {
        _Entries = _ConvertToViewModelEntries(_FolderEntry.RawEntries());
    }

    MatchProfilesEntryViewModel::MatchProfilesEntryViewModel(Model::MatchProfilesEntry matchProfilesEntry) :
        MatchProfilesEntryViewModelT<MatchProfilesEntryViewModel, NewTabMenuEntryViewModel>(Model::NewTabMenuEntryType::MatchProfiles),
        _MatchProfilesEntry{ matchProfilesEntry }
    {
    }

    hstring MatchProfilesEntryViewModel::DisplayText() const
    {
        std::wstringstream ss;
        if (const auto profileName = _MatchProfilesEntry.Name(); !profileName.empty())
        {
            ss << fmt::format(L"profile: {}, ", profileName);
        }
        if (const auto commandline = _MatchProfilesEntry.Commandline(); !commandline.empty())
        {
            ss << fmt::format(L"profile: {}, ", commandline);
        }
        if (const auto source = _MatchProfilesEntry.Source(); !source.empty())
        {
            ss << fmt::format(L"profile: {}, ", source);
        }

        // Chop off the last ", "
        auto s = ss.str();
        return winrt::hstring{ s.substr(0, s.size() - 2) };
    }

    RemainingProfilesEntryViewModel::RemainingProfilesEntryViewModel(Model::RemainingProfilesEntry remainingProfilesEntry) :
        RemainingProfilesEntryViewModelT<RemainingProfilesEntryViewModel, NewTabMenuEntryViewModel>(Model::NewTabMenuEntryType::RemainingProfiles),
        _RemainingProfilesEntry{ remainingProfilesEntry }
    {
    }
}
