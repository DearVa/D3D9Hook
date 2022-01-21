using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using TestHook;

namespace WPFD3Dhack {
	public partial class Window2 {
		public Window2() {
			Window1.Hook(new WindowInteropHelper(this).EnsureHandle(), 800, 600);
			WindowStyle = WindowStyle.None;
			Background = Brushes.Black;
			Left = 0;
			Top = 0;
			WindowStyle = WindowStyle.None;
			Background = Brushes.Black;
			ShowInTaskbar = false;
			ResizeMode = ResizeMode.NoResize;
			Visibility = Visibility.Hidden;
			InitializeComponent();
			Loaded += (_, _) => {
				Task.Run(() => {
					Thread.Sleep(500);
					Dispatcher.Invoke(() => ButtonBase_OnClick(null!, null!));
				});
			};
		}

		private void ButtonBase_OnClick(object sender, RoutedEventArgs e) {
			var buf = IntPtr.Zero;
			var sw = Stopwatch.StartNew();
			var result = Window1.CaptureLock(ref buf);
			sw.Stop();
			Trace.WriteLine($"[D3D9Hook] CaptureLock takes: {sw.ElapsedMilliseconds}ms");
			if (result == 0) {
				var size = (ulong)800 * 600 * 4;
				var bitmap = new WriteableBitmap(800, 600, 96d, 96d, PixelFormats.Bgr32, null);
				bitmap.Lock();
				unsafe {
					Buffer.MemoryCopy(buf.ToPointer(), bitmap.BackBuffer.ToPointer(), size, size);
				}
				bitmap.Unlock();
				bitmap.Freeze();
				new Window3(bitmap).ShowDialog();
			}
			Window1.CaptureUnlock();
		}
	}
}
