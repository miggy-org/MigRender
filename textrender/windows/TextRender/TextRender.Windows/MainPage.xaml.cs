using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage;
using Windows.UI.ApplicationSettings;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

namespace TextRender
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private MiggyRT.MiggyRender miggyRender = null;
        private MiggyRT.MiggySurface miggySurface = null;
        private Button btnClicked = null;

        public MainPage()
        {
            this.InitializeComponent();
            setPickerVisibility(false);

            // defaults
            btnFont.Content = MiggyParams.fontNames[13];
            btnMat.Content = MiggyParams.textMaterials[3];
            btnBackdrop.Content = MiggyParams.backdropNames[3];
            btnLights.Content = MiggyParams.lightingNames[2];
            btnOrient.Content = MiggyParams.cameraNames[2];
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            SettingsPane.GetForCurrentView().CommandsRequested += onCommandsRequested;
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            base.OnNavigatedFrom(e);
            SettingsPane.GetForCurrentView().CommandsRequested -= onCommandsRequested;
        }

        private void onCommandsRequested(SettingsPane settingsPane, SettingsPaneCommandsRequestedEventArgs e)
        {
            SettingsCommand generalCommand = new SettingsCommand("render", "Render Settings",
                (handler) =>
                {
                    //rootPage.NotifyUser("You opened the 'Defaults' SettingsFlyout.", NotifyType.StatusMessage);
                    RenderSettingsFlyout sf = new RenderSettingsFlyout();
                    sf.Show();
                });
            e.Request.ApplicationCommands.Add(generalCommand);
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

        private void EnableButtons(bool enabled)
        {
            textEdit.IsEnabled = enabled;
            btnFont.IsEnabled = enabled;
            btnMat.IsEnabled = enabled;
            btnBackdrop.IsEnabled = enabled;
            btnLights.IsEnabled = enabled;
            btnOrient.IsEnabled = enabled;
            btnRender.Content = (enabled ? "Render" : "Abort");
        }

        private async Task ShowRenderError(string errMsg)
        {
            miggyRender = null;
            EnableButtons(true);

            MessageDialog dlg = new MessageDialog(errMsg);
            await dlg.ShowAsync();
        }

        private void ClickRenderButton1(object sender, RoutedEventArgs e)
        {
            int width = 320; // (int)displayRect.ActualWidth;
            int height = 240; // (int)displayRect.ActualHeight;
            miggySurface = new MiggyRT.MiggySurface(width, height);

            ImageBrush br = new ImageBrush();
            br.ImageSource = miggySurface;
            displayRect.Fill = br;

            miggySurface.BeginDraw();
            miggySurface.TestDraw();
            miggySurface.EndDraw();
        }

        private async void ClickRenderButton(object sender, RoutedEventArgs e)
        {
            // the presence of the MiggyRender object means we are rendering
            if (miggyRender == null)
            {
                miggyRender = new MiggyRT.MiggyRender();
                EnableButtons(false);

                // load rendering settings
                ApplicationDataContainer appData = Windows.Storage.ApplicationData.Current.LocalSettings;
                int nCores = ((bool) appData.Values["multiCore"] ? 4 : 1);
                bool superSample = (bool)appData.Values["superSample"];
                bool useShadows = (bool)appData.Values["useShadows"];
                bool softShadows = (bool)appData.Values["softShadows"];
                bool autoReflect = (bool)appData.Values["autoReflect"];

                // init
                if (!miggyRender.Init(nCores, superSample, useShadows, softShadows, autoReflect))
                {
                    await ShowRenderError("Init() failed");
                    return;
                }

                // load the font model
                String fontModel = MiggyParams.fontModel(btnFont.Content.ToString());
                if (!miggyRender.SetText(textEdit.Text, fontModel))
                {
                    await ShowRenderError("SetText() failed");
                    return;
                }

                // load the backdrop model
                String backdropModel = MiggyParams.backdropModel(btnBackdrop.Content.ToString());
                if (backdropModel != "")
                {
                    double specR = 0, specG = 0, specB = 0;
                    bool bdAutoReflect = false;
                    MiggyParams.backdropParams(btnBackdrop.Content.ToString(), ref specR, ref specG, ref specB, ref bdAutoReflect);

                    if (!miggyRender.SetBackdrop(backdropModel, specR, specG, specB, bdAutoReflect))
                    {
                        await ShowRenderError("SetBackdrop() failed");
                        return;
                    }
                }

                // load the lighting model
                String lightingModel = MiggyParams.lightingModel(btnLights.Content.ToString());
                if (lightingModel != "")
                {
                    if (!miggyRender.SetLighting(lightingModel))
                    {
                        await ShowRenderError("SetLighting() failed");
                        return;
                    }
                }

                // set the orientation
                double rX = 0, rY = 0, rZ = 0;
                double tX = 0, tY = 0, tZ = 0;
                MiggyParams.orientParams(btnOrient.Content.ToString(), ref rX, ref rY, ref rZ, ref tX, ref tY, ref tZ);
                if (!miggyRender.SetOrientation(rZ, rX, rY, tX, tY, tZ))
                {
                    await ShowRenderError("SetOrientation() failed");
                    return;
                }

                // load texture/reflection maps for the text
                String texture = "", reflect = "";
                MiggyParams.textTextures(btnMat.Content.ToString(), ref texture, ref reflect);
                if (texture != "")
                {
                    if (!miggyRender.LoadImage("texture", texture, false))
                    {
                        await ShowRenderError("LoadImage(texture) failed");
                        return;
                    }
                }
                if (reflect != "")
                {
                    if (!miggyRender.LoadImage("reflect", reflect, false))
                    {
                        await ShowRenderError("LoadImage(reflect) failed");
                        return;
                    }
                }

                // load texture/reflection maps for the backdrop
                MiggyParams.backdropTextures(btnBackdrop.Content.ToString(), ref texture, ref reflect);
                if (texture != "")
                {
                    if (!miggyRender.LoadImage("bd-texture", texture, true))
                    {
                        await ShowRenderError("LoadImage(bd-texture) failed");
                        return;
                    }
                }
                if (reflect != "")
                {
                    if (!miggyRender.LoadImage("bd-reflect", reflect, true))
                    {
                        await ShowRenderError("LoadImage(bd-reflect) failed");
                        return;
                    }
                }

                // create a surface to render into
                int width = (int)(displayRect.ActualWidth + 0.5);
                int height = (int)(displayRect.ActualHeight + 0.5);
                miggySurface = new MiggyRT.MiggySurface(width, height);

                // apply the surface to an image brush, which will fill the rect
                ImageBrush br = new ImageBrush();
                br.ImageSource = miggySurface;
                br.Stretch = Stretch.Fill;
                displayRect.Fill = br;

                // start an update timer to refresh the current render
                DispatcherTimer timer = new DispatcherTimer();
                timer.Interval = TimeSpan.FromSeconds(1);
                timer.Tick += UpdateRenderTimerElapsed;

                // render
                timer.Start();
                bool ok = await miggyRender.Render(miggySurface);
                timer.Stop();

                // do a final update
                if (ok)
                {
                    miggyRender.CopyToSurface(miggySurface);
                }
                else
                {
                    await ShowRenderError("Render() failed");
                    return;
                }

                EnableButtons(true);
                miggyRender = null;
            }
            else
            {
                // signal an abort
                miggyRender.AbortRender();
            }
        }

        private void UpdateRenderTimerElapsed(object sender, object e)
        {
            if (miggyRender != null)
            {
                // if the copy fails then abort
                if (!miggyRender.CopyToSurface(miggySurface))
                    miggyRender.AbortRender();
            }
        }

        private void ClickPopupCloseButton(object sender, RoutedEventArgs e)
        {
            btnClicked.Content = popupList.SelectedItem;

            setPickerVisibility(false);
        }
    }
}
