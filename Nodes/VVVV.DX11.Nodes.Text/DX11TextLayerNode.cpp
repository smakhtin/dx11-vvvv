#include "StdAfx.h"
#include "DX11TextLayerNode.h"

namespace VVVV {
	namespace Nodes {
		namespace DX11 {

			DX11TextLayerNode::DX11TextLayerNode(IIOFactory^ factory)
			{
				this->iofactory = factory;
				factory->PluginHost->CreateTransformInput("Transform In", TSliceMode::Dynamic, TPinVisibility::True, this->FInTr);
				this->fontrenderers = gcnew	Dictionary<DX11RenderContext^, IntPtr>();
			}

			void DX11TextLayerNode::Evaluate(int SpreadMax)
			{
				this->spmax = SpreadMax;

				if (this->FOutLayer[0] == nullptr)
				{
					this->FOutLayer[0] = gcnew DX11Resource<DX11Layer^>();
				}
			}

			void DX11TextLayerNode::Update(IPluginIO^ pin, DX11RenderContext^ context)
			{
				if (!this->FOutLayer[0]->Contains(context))
				{
					this->FOutLayer[0][context] = gcnew DX11Layer();
					this->FOutLayer[0][context]->Render = gcnew RenderDelegate<DX11RenderSettings^>(this, &DX11TextLayerNode::Render);
				}

				if (!this->fontrenderers->ContainsKey(context))
				{
					FW1_FONTWRAPPERCREATEPARAMS createParams = { 0 };
					createParams.SheetMipLevels = 5;
					createParams.AnisotropicFiltering = TRUE;
					createParams.DefaultFontParams.pszFontFamily = L"Arial";
					createParams.DefaultFontParams.FontWeight = DWRITE_FONT_WEIGHT_NORMAL;
					createParams.DefaultFontParams.FontStyle = DWRITE_FONT_STYLE_NORMAL;
					createParams.DefaultFontParams.FontStretch = DWRITE_FONT_STRETCH_NORMAL;


					IFW1Factory *pFW1Factory;
					FW1CreateFactory(FW1_VERSION, &pFW1Factory);
					ID3D11Device* dev = (ID3D11Device*)context->Device->ComPointer.ToPointer();

					IFW1FontWrapper* pw;

					pFW1Factory->CreateFontWrapper(dev, NULL, &createParams, &pw);
					pFW1Factory->Release();

					this->fontrenderers->Add(context, IntPtr(pw));
				}
			}


			void DX11TextLayerNode::Destroy(IPluginIO^ pin, DX11RenderContext^ context, bool force)
			{
				if (this->FOutLayer[0]->Contains(context))
				{
					this->FOutLayer[0]->Dispose(context);
				}
			}

			void DX11TextLayerNode::Render(IPluginIO^ pin, DX11RenderContext^ context, DX11RenderSettings^ settings)
			{
				if (this->FInEnabled[0])
				{
					float w = settings->RenderWidth;
					float h = settings->RenderHeight;

					IFW1FontWrapper* fw = (IFW1FontWrapper*)this->fontrenderers[context].ToPointer();

					ID3D11Device* dev = (ID3D11Device*)context->Device->ComPointer.ToPointer();
					ID3D11DeviceContext* pContext = (ID3D11DeviceContext*)context->CurrentDeviceContext->ComPointer.ToPointer();


					int cnt;
					float* tr;
					this->FInTr->GetMatrixPointer(cnt, tr);

					SlimDX::Matrix* smp = (SlimDX::Matrix*)&tr[0];

					for (int i = 0; i < this->spmax; i++)
					{

						void* txt = (void*)Marshal::StringToHGlobalUni(this->FInString[i]);
						void* font = (void*)Marshal::StringToHGlobalUni(this->FFontInput[i]);
						float size = this->FInSize[i];
						SlimDX::Color4 c = this->FInColor[i];
						c.Red = this->FInColor[i].Blue;
						c.Blue = this->FInColor[i].Red;
						unsigned int color = c.ToArgb();


						SlimDX::Matrix preScale = SlimDX::Matrix::Scaling(1.0f, -1.0f, 1.0f);
						switch (this->FNormalizeInput[i]->Index)
						{
						case 1: preScale = SlimDX::Matrix::Scaling(1.0f / w, -1.0f / w, 1.0f); break;
						case 2: preScale = SlimDX::Matrix::Scaling(1.0f / h, -1.0f / h, 1.0f); break;
						case 3: preScale = SlimDX::Matrix::Scaling(1.0f / w, -1.0f / h, 1.0f); break;
						}

						SlimDX::Matrix sm = smp[i % this->FInTr->SliceCount];

						SlimDX::Matrix mat = SlimDX::Matrix::Multiply(preScale, sm);
						mat = SlimDX::Matrix::Multiply(mat, settings->View);
						mat = SlimDX::Matrix::Multiply(mat, settings->Projection);

						float* tr = (float*)&mat;

						FW1_RECTF rect = { 0.0f, 0.0f, 0.0f, 0.0f };

						int flag = FW1_NOWORDWRAP;

						if (this->FVerticalAlignInput[i]->Index == 0) { flag |= FW1_TOP; }
						else if (this->FVerticalAlignInput[i]->Index == 1) { flag |= FW1_VCENTER; }
						else if (this->FVerticalAlignInput[i]->Index == 2) { flag |= FW1_BOTTOM; }

						IDWriteFactory *pDWriteFactory;
						HRESULT hResult = fw->GetDWriteFactory(&pDWriteFactory);

						IDWriteTextFormat *pTextFormat;
						hResult = pDWriteFactory->CreateTextFormat(
							(WCHAR*)font,
							NULL,
							DWRITE_FONT_WEIGHT_REGULAR,
							DWRITE_FONT_STYLE_NORMAL,
							DWRITE_FONT_STRETCH_NORMAL,
							size,
							L"",
							&pTextFormat
							);

						IDWriteTextLayout *pTextLayout1;

						UINT32 length = wcslen((WCHAR*)txt);

						pDWriteFactory->CreateTextLayout((WCHAR*)txt, length, pTextFormat, this->FInWrapWidth[i], 0.0f, &pTextLayout1);

						DWRITE_WORD_WRAPPING wrap = this->FInWordWrap[i] ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP;

						pTextLayout1->SetWordWrapping(wrap);

						DWRITE_TEXT_ALIGNMENT tAlign;

						switch (this->FHorizontalAlignInput[i]->Index)
						{
						case 0:
							tAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
							break;
						case 1:
							tAlign = DWRITE_TEXT_ALIGNMENT_CENTER;
							break;
						case 2:
							tAlign = DWRITE_TEXT_ALIGNMENT_TRAILING;
							break;
						default:
							tAlign = DWRITE_TEXT_ALIGNMENT_CENTER;
							break;
						}

						pTextLayout1->SetTextAlignment(tAlign);

						DWRITE_PARAGRAPH_ALIGNMENT pAlign;

						switch (this->FVerticalAlignInput[i]->Index) {
						case 0:
							pAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
							break;
						case 1:
							pAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
							break;
						case 2:
							pAlign = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
							break;
						}

						pTextLayout1->SetParagraphAlignment(pAlign);


						DWRITE_FONT_WEIGHT weight;

						if (this->FInWeight[i] == WeightType::Bold) { weight = DWRITE_FONT_WEIGHT_BOLD; }
						else if (this->FInWeight[i] == WeightType::Thin) { weight = DWRITE_FONT_WEIGHT_THIN; }

						switch (this->FInWeight[i])
						{
						case(WeightType::Thin) :
							weight = DWRITE_FONT_WEIGHT_THIN;
							break;
						case(WeightType::ExtraLight) :
							weight = DWRITE_FONT_WEIGHT_EXTRA_LIGHT;
							break;
						case(WeightType::UltraLight) :
							weight = DWRITE_FONT_WEIGHT_ULTRA_LIGHT;
							break;
						case(WeightType::Light) :
							weight = DWRITE_FONT_WEIGHT_LIGHT;
							break;
						case(WeightType::Normal) :
							weight = DWRITE_FONT_WEIGHT_NORMAL;
							break;
						case(WeightType::Regular) :
							weight = DWRITE_FONT_WEIGHT_REGULAR;
							break;
						case(WeightType::Medium) :
							weight = DWRITE_FONT_WEIGHT_MEDIUM;
							break;
						case(WeightType::DemiBold) :
							weight = DWRITE_FONT_WEIGHT_DEMI_BOLD;
							break;
						case(WeightType::SemiBold) :
							weight = DWRITE_FONT_WEIGHT_SEMI_BOLD;
							break;
						case(WeightType::Bold) :
							weight = DWRITE_FONT_WEIGHT_BOLD;
							break;
						case(WeightType::ExtraBold) :
							weight = DWRITE_FONT_WEIGHT_EXTRA_BOLD;
							break;
						case(WeightType::UltraBold) :
							weight = DWRITE_FONT_WEIGHT_EXTRA_BOLD;
							break;
						case(WeightType::Black) :
							weight = DWRITE_FONT_WEIGHT_EXTRA_BOLD;
							break;
						case(WeightType::Heavy) :
							weight = DWRITE_FONT_WEIGHT_EXTRA_BOLD;
							break;
						case(WeightType::ExtraBlack) :
							weight = DWRITE_FONT_WEIGHT_EXTRA_BLACK;
							break;
						case(WeightType::UltraBlack) :
							weight = DWRITE_FONT_WEIGHT_ULTRA_BLACK;
							break;
						default:
							weight = DWRITE_FONT_WEIGHT_NORMAL;
							break;
						}

						DWRITE_TEXT_RANGE textRange = { 0, length };
						pTextLayout1->SetFontWeight(weight, textRange);


						/*fw->DrawString(
						pContext,
						(WCHAR*)txt,
						(WCHAR*)font,
						size,
						&rect,
						color,
						NULL,
						tr,
						flag
						);*/

						//fw->DrawTextLayout(pContext, pTextLayout1, -this->FInWrapWidth[0] / 2, 0, color, NULL, tr, 0);

						float translateX = 0.0f;


						if (this->FHorizontalAlignInput[i]->Index == 1) {
							translateX = -this->FInWrapWidth[0] / 2;
						}
						else if (this->FHorizontalAlignInput[i]->Index == 2) {
							translateX = -this->FInWrapWidth[0];
						}

						fw->DrawTextLayout(pContext, pTextLayout1, translateX, 0, color, NULL, tr, 1);

						Marshal::FreeHGlobal(System::IntPtr(txt));
						Marshal::FreeHGlobal(System::IntPtr(font));

						pTextLayout1->Release();
						pTextFormat->Release();
					}

					//Apply old states back
					context->RenderStateStack->Apply();
					context->CleanShaderStages();
				}
			}

		}
	}
}