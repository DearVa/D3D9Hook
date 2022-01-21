using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using TestHook;

namespace WPFD3Dhack {
	public partial class Window2 {
		public Window2() {
			Window1.Hook(new WindowInteropHelper(this).EnsureHandle());
			//WindowStyle = WindowStyle.None;
			Background = Brushes.Black;
			//AllowsTransparency = true;
			//ShowInTaskbar = false;
			//SizeToContent = SizeToContent.WidthAndHeight;
			ResizeMode = ResizeMode.NoResize;
			//Visibility = Visibility.Hidden;
			InitializeComponent();
		}

		private void ButtonBase_OnClick(object sender, RoutedEventArgs e) {
			InvalidateVisual();
			uint width = 0, height = 0;
			var buf = IntPtr.Zero;
			var result = Window1.Capture(ref width, ref height, ref buf);
			if (result == 0) {
				Trace.WriteLine($"[D3D9Hook] Width: {width}, Height: {height}, buf: {buf}");
				var size = (ulong)width * height * 4;
				var bitmap = new WriteableBitmap((int)width, (int)height, 96d, 96d, PixelFormats.Bgr32, null);
				bitmap.Lock();
				unsafe {
					Buffer.MemoryCopy(buf.ToPointer(), bitmap.BackBuffer.ToPointer(), size, size);
				}
				bitmap.Unlock();
				bitmap.Freeze();
				new Window3(bitmap).ShowDialog();
			}
		}
	}
}
