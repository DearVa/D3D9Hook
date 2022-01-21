using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Media3D;

namespace WPFD3Dhack {
	public partial class Window1 {
		[DllImport("D3D9Hook")]
		public static extern long Hook(IntPtr hwnd);

		[DllImport("D3D9Hook")]
		private static extern bool Unhook();

		[DllImport("D3D9Hook")]
		private static extern bool IsCaptureReady();

		[DllImport("D3D9Hook")]
		public static extern int Capture(ref uint width, ref uint height, ref IntPtr buf);

		private int width, height;
		private IntPtr buf;

		public Window1() {
			InitializeComponent();
			Loaded += Window1_Loaded;
		}

		private void Window1_Loaded(object sender, RoutedEventArgs e) {
			//Initialize();
			// Trace.WriteLine(IsCaptureReady());
			new Window2().ShowDialog();
		}

		private bool rendered;

		protected override void OnRender(DrawingContext drawingContext) {
			base.OnRender(drawingContext);
			if (!rendered) {
				width = (int)ActualWidth;
				height = (int)ActualHeight;
				rendered = true;
			}
		}

		private void ButtonBase_OnClick(object sender, RoutedEventArgs e) {
			InvalidateVisual();
		}
	}
}
