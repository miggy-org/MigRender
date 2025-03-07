// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage;
using Windows.System;
using Windows.UI.ApplicationSettings;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The SettingsFlyout item template is documented at http://go.microsoft.com/fwlink/?LinkId=273769

namespace TextRender
{
    public sealed partial class RenderSettingsFlyout : SettingsFlyout
    {
        public RenderSettingsFlyout()
        {
            this.InitializeComponent();

            ApplicationDataContainer appData = Windows.Storage.ApplicationData.Current.LocalSettings;
            checkMultiCore.IsChecked = (bool)appData.Values[checkMultiCore.Tag.ToString()];
            checkSuperSample.IsChecked = (bool)appData.Values[checkSuperSample.Tag.ToString()];
            checkShadows.IsChecked = (bool)appData.Values[checkShadows.Tag.ToString()];
            checkSoftShadows.IsChecked = (bool)appData.Values[checkSoftShadows.Tag.ToString()];
            checkReflections.IsChecked = (bool)appData.Values[checkReflections.Tag.ToString()];

            UpdateSoftShadowsCheckbox();
        }

        private void UpdateSoftShadowsCheckbox()
        {
            bool enabled = (checkShadows.IsChecked == true ? true : false);
            checkSoftShadows.IsChecked = (checkSoftShadows.IsChecked & enabled);
            checkSoftShadows.IsEnabled = enabled;
        }

        private void checkBox_Click(object sender, RoutedEventArgs e)
        {
            CheckBox checkBox = sender as CheckBox;

            ApplicationDataContainer appData = Windows.Storage.ApplicationData.Current.LocalSettings;
            appData.Values[checkBox.Tag.ToString()] = checkBox.IsChecked;

            UpdateSoftShadowsCheckbox();
        }
    }
}
