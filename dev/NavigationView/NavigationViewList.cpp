﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "NavigationViewList.h"
#include "NavigationViewItem.h"
#include "NavigationView.h"

CppWinRTActivatableClassWithBasicFactory(NavigationViewList);

NavigationViewList::NavigationViewList()
{
}

// IItemsControlOverrides

winrt::DependencyObject NavigationViewList::GetContainerForItemOverride()
{
    return winrt::make<NavigationViewItem>();
}

bool NavigationViewList::IsItemItsOwnContainerOverride(winrt::IInspectable const& args)
{
    bool isItemItsOwnContainerOverride = false;
    if (args)
    {
        auto nvib = args.try_as<winrt::NavigationViewItemBase>();
        if (nvib && nvib != m_lastItemCalledInIsItemItsOwnContainerOverride.get())
        {
            m_lastItemCalledInIsItemItsOwnContainerOverride.set(nvib);
        }
        isItemItsOwnContainerOverride = static_cast<bool>(nvib);
    }
    return isItemItsOwnContainerOverride;
}

void NavigationViewList::ClearContainerForItemOverride(winrt::DependencyObject const& element, winrt::IInspectable const& item)
{
    if (auto itemContainer = element.try_as<winrt::NavigationViewItem>())
    {
        auto itemContainerImplementation = winrt::get_self<NavigationViewItem>(itemContainer);
        itemContainerImplementation->ClearIsContentChangeHandlingDelayedForTopNavFlag();
        itemContainerImplementation->SetDepth(0);
        itemContainerImplementation->IsSelected(false);
    }
    __super::PrepareContainerForItemOverride(element, item);
}

void NavigationViewList::PrepareContainerForItemOverride(winrt::DependencyObject const& element, winrt::IInspectable const& item)
{
    auto nvNode = winrt::get_self<TreeViewNode>(NodeFromContainer(element));
    if (auto itemContainer = element.try_as<winrt::NavigationViewItemBase>())
    {
        winrt::get_self<NavigationViewItemBase>(itemContainer)->Position(m_navigationViewListPosition);
        
        winrt::get_self<NavigationViewItemBase>(itemContainer)->SetDepth(nvNode->Depth());
    }
    if (auto itemContainer = element.try_as<winrt::NavigationViewItem>())
    {
        itemContainer.UseSystemFocusVisuals(m_showFocusVisual);
        winrt::get_self<NavigationViewItem>(itemContainer)->ClearIsContentChangeHandlingDelayedForTopNavFlag();

        bool isChildSelected = nvNode->SelectionState() == TreeNodeSelectionState::PartialSelected ? true : false;
        winrt::get_self<NavigationViewItem>(itemContainer)->IsChildSelected(isChildSelected);
        
    }
    __super::PrepareContainerForItemOverride(element, item);
}

void NavigationViewList::SetNavigationViewListPosition(NavigationViewListPosition navigationViewListPosition)
{
    m_navigationViewListPosition = navigationViewListPosition;
    PropagateChangeToAllContainers<winrt::NavigationViewItemBase>(
        [&navigationViewListPosition](const winrt::NavigationViewItemBase& container)
            {
                winrt::get_self<NavigationViewItemBase>(container)->Position(navigationViewListPosition);
            });
}

void NavigationViewList::SetShowFocusVisual(bool showFocus)
{
    m_showFocusVisual = showFocus;
    PropagateChangeToAllContainers<winrt::NavigationViewItem>(
        [showFocus](const winrt::NavigationViewItem& container)
    {
        container.UseSystemFocusVisuals(showFocus);
    });
}

void NavigationViewList::SetNavigationViewParent(winrt::NavigationView const& navigationView)
{
    m_navigationView = winrt::make_weak(navigationView);
}

// IControlOverrides
void NavigationViewList::OnKeyDown(winrt::KeyRoutedEventArgs const& eventArgs)
{
    auto key = eventArgs.Key();

    if (key == winrt::VirtualKey::GamepadLeftShoulder
        || key == winrt::VirtualKey::GamepadRightShoulder
        || key == winrt::VirtualKey::GamepadLeftTrigger
        || key == winrt::VirtualKey::GamepadRightTrigger)
    {
        // We need to return at this point to prevent ListView from handling page up / page down
        // when any of these four keys get triggered. The reason is that it would navigate to the
        // first or last item in the list and handle the event, preventing NavigationView
        // to do its KeyDown handling afterwards.
        return;
    }

    __super::OnKeyDown(eventArgs);
}

winrt::NavigationView NavigationViewList::GetNavigationViewParent()
{
    return m_navigationView.get();
}

winrt::NavigationViewItemBase NavigationViewList::GetLastItemCalledInIsItemItsOwnContainerOverride()
{
    return m_lastItemCalledInIsItemItsOwnContainerOverride.get();
}

template<typename T> 
void NavigationViewList::PropagateChangeToAllContainers(std::function<void(typename T& container)> function)
{
    if (auto items = Items())
    {
        auto size = static_cast<int>(items.Size());
        for (int i = 0; i < size; i++)
        {
            auto container = ContainerFromIndex(i);
            if (container)
            {
                auto itemContainer = container.try_as<typename T>();
                if (itemContainer)
                {
                    function(itemContainer);
                }
            }
        }
    }
}

com_ptr<ViewModel> NavigationViewList::ListViewModel() const
{
    return m_viewModel.get();
}

void NavigationViewList::ListViewModel(com_ptr<ViewModel> viewModel)
{
    m_viewModel.set(viewModel);
}

winrt::TreeViewNode NavigationViewList::NodeAtFlatIndex(int index) const
{
    return ListViewModel()->GetNodeAt(index);
}

winrt::TreeViewNode NavigationViewList::NodeFromContainer(winrt::DependencyObject const& container)
{
    int index = IndexFromContainer(container);
    if (index >= 0 && index < static_cast<int32_t>(ListViewModel()->Size()))
    {
        return NodeAtFlatIndex(index);
    }
    return nullptr;
}

winrt::DependencyObject NavigationViewList::ContainerFromNode(winrt::TreeViewNode const& node)
{
    return ContainerFromItem(node.Content());
}
