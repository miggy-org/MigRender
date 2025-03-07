using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using TextRender.Common;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Graphics.Display;
using Windows.Storage;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkID=390556

namespace TextRender
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class RenderPage : Page
    {
        public class RenderParams
        {
            public string text;
            public string fontName;
            public string fontMat;
            public string backdrop;
            public string lights;
            public string orientation;
        }

        private NavigationHelper navigationHelper;

        private MiggyRT.MiggyRender miggyRender = null;
        private MiggyRT.MiggySurface miggySurface = null;
        private DispatcherTimer startTimer = null;
        private RenderParams renderParams = null;

        public RenderPage()
        {
            this.InitializeComponent();

            this.navigationHelper = new NavigationHelper(this);
            this.navigationHelper.LoadState += this.NavigationHelper_LoadState;
            this.navigationHelper.SaveState += this.NavigationHelper_SaveState;
        }

        #region NavigationHelper registration

        /// <summary>
        /// The methods provided in this section are simply used to allow
        /// NavigationHelper to respond to the page's navigation methods.
        /// <para>
        /// Page specific logic should be placed in event handlers for the  
        /// <see cref="NavigationHelper.LoadState"/>
        /// and <see cref="NavigationHelper.SaveState"/>.
        /// The navigation parameter is available in the LoadState method 
        /// in addition to page state preserved during an earlier session.
        /// </para>
        /// </summary>
        /// <param name="e">Provides data for navigation methods and event
        /// handlers that cannot cancel the navigation request.</param>
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            DisplayInformation.AutoRotationPreferences = DisplayOrientations.Landscape;
            this.navigationHelper.OnNavigatedTo(e);
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            this.navigationHelper.OnNavigatedFrom(e);

            if (miggyRender != null)
                miggyRender.AbortRender();
            DisplayInformation.AutoRotationPreferences = DisplayOrientations.Portrait;
        }

        #endregion

        #region NavigationHelper functions

        /// <summary>
        /// Populates the page with content passed during navigation.  Any saved state is also
        /// provided when recreating a page from a prior session.
        /// </summary>
        /// <param name="sender">
        /// The source of the event; typically <see cref="NavigationHelper"/>
        /// </param>
        /// <param name="e">Event data that provides both the navigation parameter passed to
        /// <see cref="Frame.Navigate(Type, Object)"/> when this page was initially requested and
        /// a dictionary of state preserved by this page during an earlier
        /// session.  The state will be null the first time a page is visited.</param>
        private void NavigationHelper_LoadState(object sender, LoadStateEventArgs e)
        {
            renderParams = e.NavigationParameter as RenderParams;
        }

        /// <summary>
        /// Preserves state associated with this page in case the application is suspended or the
        /// page is discarded from the navigation cache.  Values must conform to the serialization
        /// requirements of <see cref="SuspensionManager.SessionState"/>.
        /// </summary>
        /// <param name="sender">The source of the event; typically <see cref="NavigationHelper"/></param>
        /// <param name="e">Event data that provides an empty dictionary to be populated with
        /// serializable state.</param>
        private void NavigationHelper_SaveState(object sender, SaveStateEventArgs e)
        {
        }

        #endregion

        protected override Size MeasureOverride(Size availableSize)
        {
            // rendering should start 1 second after the final measurement
            if (startTimer == null)
            {
                // start a new timer
                startTimer = new DispatcherTimer();
                startTimer.Interval = TimeSpan.FromSeconds(1);
                startTimer.Tick += StartRenderTimer;
            }
            else
            {
                // restart the timer
                startTimer.Stop();
                startTimer.Start();
            }

            return base.MeasureOverride(availableSize);
        }

        private void StartRenderTimer(object sender, object e)
        {
            startTimer.Stop();
            startTimer = null;

            if (miggyRender == null && renderParams != null)
            {
                int dimen1 = (int)(displayRect.ActualWidth + 1);
                int dimen2 = (int)(displayRect.ActualHeight + 1);
                int width = Math.Max(dimen1, dimen2);
                int height = Math.Min(dimen1, dimen2);

                StartRender(renderParams, width, height);
            }
        }

        private async Task ShowRenderError(string errMsg)
        {
            miggyRender = null;

            MessageDialog dlg = new MessageDialog(errMsg);
            await dlg.ShowAsync();
        }

        private async void StartRender(RenderParams rp, int width, int height)
        {
            // the presence of the MiggyRender object means we are rendering
            if (miggyRender == null)
            {
                miggyRender = new MiggyRT.MiggyRender();

                // load rendering settings
                ApplicationDataContainer appData = Windows.Storage.ApplicationData.Current.LocalSettings;
                int nCores = ((bool)appData.Values["multiCore"] ? 4 : 1);
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
                String fontModel = MiggyParams.fontModel(rp.fontName);
                if (!miggyRender.SetText(rp.text, fontModel))
                {
                    await ShowRenderError("SetText() failed");
                    return;
                }

                // load the backdrop model
                String backdropModel = MiggyParams.backdropModel(rp.backdrop);
                if (backdropModel != "")
                {
                    double specR = 0, specG = 0, specB = 0;
                    bool bdAutoReflect = false;
                    MiggyParams.backdropParams(rp.backdrop, ref specR, ref specG, ref specB, ref bdAutoReflect);

                    if (!miggyRender.SetBackdrop(backdropModel, specR, specG, specB, bdAutoReflect))
                    {
                        await ShowRenderError("SetBackdrop() failed");
                        return;
                    }
                }

                // load the lighting model
                String lightingModel = MiggyParams.lightingModel(rp.lights);
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
                MiggyParams.orientParams(rp.orientation, ref rX, ref rY, ref rZ, ref tX, ref tY, ref tZ);
                if (!miggyRender.SetOrientation(rZ, rX, rY, tX, tY, tZ))
                {
                    await ShowRenderError("SetOrientation() failed");
                    return;
                }

                // load texture/reflection maps for the text
                String texture = "", reflect = "";
                MiggyParams.textTextures(rp.fontMat, ref texture, ref reflect);
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
                MiggyParams.backdropTextures(rp.backdrop, ref texture, ref reflect);
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
                miggySurface = new MiggyRT.MiggySurface(width, height);

                // apply the surface to an image brush, which will fill the rect
                ImageBrush br = new ImageBrush();
                br.ImageSource = miggySurface;
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
    }
}
