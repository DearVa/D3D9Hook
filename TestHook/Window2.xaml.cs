using System;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;

namespace WPFD3Dhack {
	public partial class Window2 {
		public Window2() {
			WindowStyle = WindowStyle.None;
			Background = Brushes.Black;
			//AllowsTransparency = true;
			ShowInTaskbar = false;
			SizeToContent = SizeToContent.WidthAndHeight;
			ResizeMode = ResizeMode.NoResize;
			Visibility = Visibility.Hidden;
			InitializeComponent();
			Window1.Hook(new WindowInteropHelper(this).EnsureHandle());
		}

		private void ButtonBase_OnClick(object sender, RoutedEventArgs e) {
			InvalidateVisual();
			uint width = 0, height = 0;
			var buf = IntPtr.Zero;
			var result = Window1.Capture(ref width, ref height, ref buf);
			if (result == 0) {
				MessageBox.Show("Success");
			}
		}
	}
}
