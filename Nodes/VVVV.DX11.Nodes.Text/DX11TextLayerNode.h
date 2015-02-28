#pragma once

#include "FW1FontWrapper.h"
#include <map>

using namespace VVVV::PluginInterfaces::V2;
using namespace VVVV::PluginInterfaces::V1;

using namespace FeralTic::DX11;
using namespace FeralTic::DX11::Resources;

using namespace VVVV::DX11;

using namespace System::Runtime::InteropServices;
using namespace System::Collections::Generic;
using namespace System;
using namespace System::ComponentModel::Composition;

namespace VVVV {
	namespace Nodes {
		namespace DX11 {

			public enum class WeightType
			{
				Thin,
				ExtraLight,
				UltraLight,
				Light,
				Normal,
				Regular,
				Medium,
				DemiBold,
				SemiBold,
				Bold,
				ExtraBold,
				UltraBold,
				Black,
				Heavy,
				ExtraBlack,
				UltraBlack
			};

			public enum StyleType
			{
				Normal,
				Oblique,
				Italic
			};

			[PluginInfo(Name = "Text", Author = "vux", Category = "DX11.Layer", Version = "")]
			public ref class DX11TextLayerNode : public IPluginEvaluate, IDX11LayerProvider
			{
			public:
				[ImportingConstructor()]
				DX11TextLayerNode(IIOFactory^ factory);
				virtual void Evaluate(int SpreadMax);
				virtual void Update(IPluginIO^ pin, DX11RenderContext^ OnDevice);
				virtual void Destroy(IPluginIO^ pin, DX11RenderContext^ OnDevice, bool force);
			private:
				ITransformIn^ FInTr;

				[Input("String", DefaultString = "DX11")]
				ISpread<System::String^>^ FInString;

				[Input("Font", EnumName = "SystemFonts")]
				ISpread<EnumEntry^>^ FFontInput;

				[Input("Italic")]
				ISpread<bool>^ FItalicInput;

				[Input("Bold")]
				IDiffSpread<bool>^ FBoldInput;

				[Input("Size")]
				ISpread<float>^ FInSize;

				[Input("Color")]
				ISpread<SlimDX::Color4>^ FInColor;

				[Input("Horizontal Align", EnumName = "HorizontalAlign")]
				ISpread<EnumEntry^>^ FHorizontalAlignInput;

				[Input("Vertical Align", EnumName = "VerticalAlign")]
				ISpread<EnumEntry^>^ FVerticalAlignInput;

				[Input("Normalize", EnumName = "Normalize")]
				ISpread<EnumEntry^>^ FNormalizeInput;

				[Input("Enabled", IsSingle = true, DefaultValue = 1)]
				ISpread<bool>^ FInEnabled;

				[Input("Word Wrap", IsSingle = true, DefaultValue = 0)]
				ISpread<bool>^ FInWordWrap;

				[Input("Width [px] : Multiline Mode", IsSingle = true, DefaultValue = 300.0f)]
				ISpread<float>^ FInWrapWidth;

				[Input("Weight", IsSingle = true)]
				ISpread<WeightType>^ FInWeight;


				[Output("Layer", IsSingle = true)]
				ISpread<DX11Resource<DX11Layer^>^>^ FOutLayer;

				void Render(IPluginIO^ pin, DX11RenderContext^ ctx, DX11RenderSettings^ settings);

				Dictionary<DX11RenderContext^, IntPtr>^ fontrenderers;
				int spmax;

				IIOFactory^ iofactory;
			};
		}
	}
}