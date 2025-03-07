using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Graphics.Display;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

namespace TextRender
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private Button btnClicked = null;

        public MainPage()
        {
            this.InitializeComponent();

            this.NavigationCacheMode = NavigationCacheMode.Required;

            // defaults
            btnFont.Content = MiggyParams.fontNames[13];
            btnMat.Content = MiggyParams.textMaterials[3];
            btnBackdrop.Content = MiggyParams.backdropNames[3];
            btnLights.Content = MiggyParams.lightingNames[2];
            btnOrient.Content = MiggyParams.cameraNames[2];
            setPickerVisibility(false);
        }

        /// <summary>
        /// Invoked when this page is about to be displayed in a Frame.
        /// </summary>
        /// <param name="e">Event data that describes how this page was reached.
        /// This parameter is typically used to configure the page.</param>
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            // TODO: Prepare page for display here.

            // TODO: If your application contains multiple pages, ensure that you are
            // handling the hardware Back button by registering for the
            // Windows.Phone.UI.Input.HardwareButtons.BackPressed event.
            // If you are using the NavigationHelper provided by some templates,
            // this event is handled for you.
        }

        private void setPickerVisibility(bool isOpen)
        {
            popupBackground.Visibility = (isOpen ? Visibility.Visible : Visibility.Collapsed);
            popupPicker.IsOpen = isOpen;
        }

        private void populatePicker(String[] items, object sender)
        {
            btnClicked = (Button)sender;

            popupList.Items.Clear();
            for (int i = 0; i < items.Length; i++)
                popupList.Items.Add(items[i]);
            popupList.SelectedItem = btnClicked.Content.ToString();

            setPickerVisibility(true);
        }

        private void ClickFontButton(object sender, RoutedEventArgs e)
        {
            populatePicker(MiggyParams.fontNames, sender);
        }

        private void ClickTextMaterialButton(object sender, RoutedEventArgs e)
        {
            populatePicker(MiggyParams.textMaterials, sender);
        }

        private void ClickBackdropButton(object sender, RoutedEventArgs e)
        {
            populatePicker(MiggyParams.backdropNames, sender);
        }

        private void ClickLightsButton(object sender, RoutedEventArgs e)
        {
            populatePicker(MiggyParams.lightingNames, sender);
        }

        private void ClickCameraButton(object sender, RoutedEventArgs e)
        {
            populatePicker(MiggyParams.cameraNames, sender);
        }

        private void ClickRenderButton(object sender, RoutedEventArgs e)
        {
            if (this.Frame != null)
            {
                RenderPage.RenderParams rp = new RenderPage.RenderParams();
                rp.text = textEdit.Text;
                rp.fontName = btnFont.Content.ToString();
                rp.fontMat = btnMat.Content.ToString();
                rp.backdrop = btnBackdrop.Content.ToString();
                rp.lights = btnLights.Content.ToString();
                rp.orientation = btnOrient.Content.ToString();

                this.Frame.Navigate(typeof(RenderPage), rp);
            }
        }

        private void ClickPopupCloseButton(object sender, RoutedEventArgs e)
        {
            btnClicked.Content = popupList.SelectedItem;

            setPickerVisibility(false);
        }

        private void Settings_Click(object sender, RoutedEventArgs e)
        {
            if (this.Frame != null)
                this.Frame.Navigate(typeof(RenderSettings));
        }
    }
}
