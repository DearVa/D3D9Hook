using System;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Media;

namespace WPFD3Dhack {
	public partial class Window1 {
		[DllImport("D3D9Hook")]
		public static extern long Hook(IntPtr hwnd, uint width, uint height);

		[DllImport("D3D9Hook")]
		private static extern bool Unhook();

		[DllImport("D3D9Hook")]
		private static extern bool IsCaptureReady();

		[DllImport("D3D9Hook")]
		public static extern int CaptureLock(ref IntPtr buf);
		
		[DllImport("D3D9Hook")]
		public static extern bool CaptureUnlock();

		public Window1() {
			InitializeComponent();
			Loaded += Window1_Loaded;
		}

		private void Window1_Loaded(object sender, RoutedEventArgs e) {
			
		}

		private void ButtonBase_OnClick(object sender, RoutedEventArgs e) {
			InvalidateVisual();
			new Window2().Show();
		}

		protected override void OnClosed(EventArgs e) {
			base.OnClosed(e);
			Application.Current.Shutdown();
		}
	}
}
