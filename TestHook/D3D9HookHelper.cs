using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace TestHook {
	internal static class D3D9HookHelper {
		[DllImport("D3D9Hook")]
		private static extern long Hook(IntPtr hwnd, uint width, int height);

		[DllImport("D3D9Hook")]
		private static extern bool Unhook();

		[DllImport("D3D9Hook")]
		private static extern bool IsCaptureReady();

		[DllImport("D3D9Hook")]
		private static extern int CaptureLock(ref IntPtr buf);

		public static void Hook() {

		}
	}
}
