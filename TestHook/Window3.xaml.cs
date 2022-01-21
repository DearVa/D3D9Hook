using System.Windows.Media;

namespace TestHook {
	public partial class Window3 {
		public Window3(ImageSource image) {
			InitializeComponent();
			Image.Source = image;
			Width = image.Width;
			Height = image.Height;
		}
	}
}
