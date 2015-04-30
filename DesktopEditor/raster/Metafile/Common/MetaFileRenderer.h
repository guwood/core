#ifndef _METAFILE_COMMON_METAFILERENDERER_H
#define _METAFILE_COMMON_METAFILERENDERER_H


#include "../../../graphics/IRenderer.h"
#include "../../../graphics/structures.h"
#include "../../../graphics/Image.h"
#include "../../../raster/ImageFileFormatChecker.h"
#include "../../../raster/BgraFrame.h"
#include "../../../graphics/AggPlusEnums.h"

#include "IOutputDevice.h"
#include "MetaFile.h"
#include "MetaFileTypes.h"
#include "MetaFileObjects.h"
#include "../Common.h"

namespace MetaFile
{
	class CMetaFileRenderer : public IOutputDevice
	{

	public:
		CMetaFileRenderer(IMetaFileBase *pFile, IRenderer *pRenderer, double dX, double dY, double dWidth, double dHeight)
		{
			m_pFile = pFile;

			m_dX = dX;
			m_dY = dY;
			m_dW = dWidth;
			m_dH = dHeight;

			m_pRenderer = NULL;

			if (!pRenderer)
				return;

			m_pRenderer = pRenderer;

			TRect* pBounds = m_pFile->GetDCBounds();
			int nL = pBounds->nLeft;
			int nR = pBounds->nRight;
			int nT = pBounds->nTop;
			int nB = pBounds->nBottom;

			m_dScaleX = (nR - nL <= 0) ? 1 : m_dW / (double)(nR - nL);
			m_dScaleY = (nB - nT <= 0) ? 1 : m_dH / (double)(nB - nT);

			m_bStartedPath = false;
		}
		~CMetaFileRenderer()
		{
		}

		void Begin()
		{
		}
		void End()
		{
			CheckEndPath();
		}

		void DrawBitmap(int lX, int lY, int lW, int lH, BYTE* pBuffer, unsigned int ulWidth, unsigned int ulHeight)
		{
			CheckEndPath();

			UpdateTransform();
			UpdateClip();

			Aggplus::CImage oImage;
			BYTE* pBufferPtr = new BYTE[4 * ulWidth * ulHeight];
			oImage.Create(pBufferPtr, ulWidth, ulHeight, 4 * ulWidth);

			for (int nIndex = 0, nSize = 4 * ulWidth * ulHeight; nIndex < nSize; nIndex += 4)
			{
				pBufferPtr[0] = (unsigned char)pBuffer[nIndex + 0];
				pBufferPtr[1] = (unsigned char)pBuffer[nIndex + 1];
				pBufferPtr[2] = (unsigned char)pBuffer[nIndex + 2];
				pBufferPtr[3] = (unsigned char)pBuffer[nIndex + 3];
				pBufferPtr += 4;
			}

			TPointD oTL = TranslatePoint(lX, lY);
			TPointD oBR = TranslatePoint(lX + lW, lY + lH);
			m_pRenderer->DrawImage(&oImage, oTL.x, oTL.y, oBR.x - oTL.x, oBR.y - oTL.y);
		}
		void DrawText(std::wstring& wsText, unsigned int ulCharsCount, int lX, int lY, int nTextW, bool bWithOutLast)
		{
			CheckEndPath();

			UpdateTransform();
			UpdateClip();

			IFont* pFont = m_pFile->GetFont();
			if (!pFont)
				return;

			int lLogicalFontHeight = pFont->GetHeight();
			if (lLogicalFontHeight < 0)
				lLogicalFontHeight = -lLogicalFontHeight;
			if (lLogicalFontHeight < 0.01)
				lLogicalFontHeight = 18;

			double dFontHeight = lLogicalFontHeight * m_dScaleY * m_pFile->GetPixelHeight() / 25.4 * 72;

			std::wstring wsFaceName = pFont->GetFaceName();
			m_pRenderer->put_FontName(wsFaceName);
			m_pRenderer->put_FontSize(dFontHeight);

			int lStyle = 0;
			if (pFont->GetWeight() > 550)
				lStyle |= 0x01;
			if (pFont->IsItalic())
				lStyle |= 0x02;
			if (pFont->IsUnderline())
				lStyle |= (1 << 2);
			if (pFont->IsStrikeOut())
				lStyle |= (1 << 7);

			m_pRenderer->put_FontStyle(lStyle);

			double dTheta = -((((double)pFont->GetEscapement()) / 10) * M_PI / 180);

			double dCosTheta = (float)cos(dTheta);
			double dSinTheta = (float)sin(dTheta);

			float fL = 0, fT = 0, fW = 0, fH = 0;
			float fUndX1 = 0, fUndY1 = 0, fUndX2 = 0, fUndY2 = 0, fUndSize = 1;
			m_pRenderer->put_FontCharSpace(0);
			CFontManager* pFontManager = m_pFile->GetFontManager();
			if (pFontManager)
			{
				pFontManager->SetCharSpacing(0);
				pFontManager->LoadFontByName(wsFaceName, dFontHeight, lStyle, 72, 72);
				double dMmToPt = 25.4 / 72;
				double dFHeight  = dFontHeight * pFontManager->m_pFont->GetHeight() / pFontManager->m_pFont->m_lUnits_Per_Em * dMmToPt;
				double dFDescent = dFontHeight * pFontManager->m_pFont->GetDescender() / pFontManager->m_pFont->m_lUnits_Per_Em * dMmToPt;
				double dFAscent  = dFHeight - std::abs(dFDescent);

				// ���������� ��������� �������������
				pFontManager->GetUnderline(&fUndX1, &fUndY1, &fUndX2, &fUndY2, &fUndSize);
				fUndX1 *= (float)dMmToPt; fUndY1 *= (float)dMmToPt;
				fUndX2 *= (float)dMmToPt; fUndY2 *= (float)dMmToPt;
				fUndSize *= (float)dMmToPt / 2;

				if (0 != nTextW && ulCharsCount > 1)
				{
					std::wstring wsTempText = wsText;
					if (bWithOutLast)
						wsTempText.erase(ulCharsCount - 1);

					pFontManager->LoadString1(wsTempText, 0, 0);

					TBBox oBox = pFontManager->MeasureString2();
					fL = (float)dMmToPt * (oBox.fMinX);
					fT = (float)dMmToPt * (oBox.fMinY);
					fW = (float)dMmToPt * (oBox.fMaxX - oBox.fMinX);
					fH = (float)dMmToPt * (oBox.fMaxY - oBox.fMinY);

					double dTextW = nTextW * m_dScaleX * m_pFile->GetPixelWidth();
					double dCharSpace = (dTextW - fW) / (ulCharsCount - 1) * 72 / 25.4;

					if (dCharSpace > 0.001 || dCharSpace < -0.001)
					{
						pFontManager->SetCharSpacing(dCharSpace);
						double dRendDpiX;
						m_pRenderer->get_DpiX(&dRendDpiX);
						m_pRenderer->put_FontCharSpace(dCharSpace * 25.4 / 72);
					}

					pFontManager->LoadString1(wsText, 0, 0);
					oBox = pFontManager->MeasureString2();
					fL = (float)dMmToPt * (oBox.fMinX);
					fT = (float)dMmToPt * (oBox.fMinY);
					fW = (float)dMmToPt * (oBox.fMaxX - oBox.fMinX);
					fH = (float)dMmToPt * (oBox.fMaxY - oBox.fMinY);
				}
				else
				{
					pFontManager->LoadString1(wsText, 0, 0);

					TBBox oBox = pFontManager->MeasureString2();
					fL = (float)dMmToPt * (oBox.fMinX);
					fT = (float)dMmToPt * (oBox.fMinY);
					fW = (float)dMmToPt * (oBox.fMaxX - oBox.fMinX);
					fH = (float)dMmToPt * (oBox.fMaxY - oBox.fMinY);
				}

				if (std::abs(fT) < dFAscent)
				{
					if (fT < 0)
						fT = (float)-dFAscent;
					else
						fT = (float)dFAscent;
				}

				if (fH < dFHeight)
					fH = (float)dFHeight;
			}

			TPointD oTextPoint = TranslatePoint(lX, lY);
			double dX = oTextPoint.x;
			double dY = oTextPoint.y;

			// ������ ��������� ����� ������
			unsigned int ulTextAlign = m_pFile->GetTextAlign();
			if (ulTextAlign & TA_BASELINE)
			{
				// ������ �� ������
			}
			else if (ulTextAlign & TA_BOTTOM)
			{
				float fTemp = -(-fT + fH);

				dX += -fTemp * dSinTheta;
				dY +=  fTemp * dCosTheta;
			}
			else // if (ulTextAlign & TA_TOP)
			{
				float fTemp = -fT;

				dX += -fTemp * dSinTheta;
				dY +=  fTemp * dCosTheta;
			}

			if (ulTextAlign & TA_CENTER)
			{
				dX += -fW / 2 * dCosTheta;
				dY += -fW / 2 * dSinTheta;
			}
			else if (ulTextAlign & TA_RIGHT)
			{
				dX += -fW * dCosTheta;
				dY += -fW * dSinTheta;
			}
			else //if (ulTextAlign & TA_LEFT)
			{
				// ������ �� ������
			}

			if (pFont->IsUnderline())
			{
				fUndX1 += (float)dX;
				fUndX2 += (float)dX;
				fUndY1 += (float)dY;
				fUndY2 += (float)dY;
			}

			bool bChangeCTM = false;
			if (0 != pFont->GetEscapement())
			{
				// TODO: ��� ���������� ������ �������� shEscapement, ��� ����� ����������� �������� Orientation
				m_pRenderer->SetTransform(dCosTheta, dSinTheta, -dSinTheta, dCosTheta, dX - dX * dCosTheta + dY * dSinTheta, dY - dX * dSinTheta - dY * dCosTheta);
				bChangeCTM = true;
			}

			// ��� ������ �������� ��� ������
			if (OPAQUE == m_pFile->GetTextBgMode())
			{
				m_pRenderer->put_BrushType(c_BrushTypeSolid);
				m_pRenderer->put_BrushAlpha1(255);
				m_pRenderer->put_BrushColor1(m_pFile->GetTextBgColor());

				m_pRenderer->BeginCommand(c_nPathType);
				m_pRenderer->PathCommandStart();
				m_pRenderer->PathCommandMoveTo(dX + fL, dY + fT);
				m_pRenderer->PathCommandLineTo(dX + fL + fW, dY + fT);
				m_pRenderer->PathCommandLineTo(dX + fL + fW, dY + fT + fH);
				m_pRenderer->PathCommandLineTo(dX + fL, dY + fT + fH);
				m_pRenderer->PathCommandClose();
				m_pRenderer->DrawPath(c_nWindingFillMode);
				m_pRenderer->EndCommand(c_nPathType);
				m_pRenderer->PathCommandEnd();
			}

			// �������� ������������� 
			if (pFont->IsUnderline())
			{
				m_pRenderer->put_PenSize((double)fUndSize);
				m_pRenderer->put_PenLineEndCap(0);
				m_pRenderer->put_PenLineStartCap(0);

				m_pRenderer->BeginCommand(c_nPathType);
				m_pRenderer->PathCommandStart();
				m_pRenderer->PathCommandMoveTo(fUndX1, fUndY1);
				m_pRenderer->PathCommandLineTo(fUndX2, fUndY2);
				m_pRenderer->DrawPath(c_nStroke);
				m_pRenderer->EndCommand(c_nPathType);
				m_pRenderer->PathCommandEnd();
			}

			// ��������� ���� ������
			m_pRenderer->put_BrushType(c_BrushTypeSolid);
			m_pRenderer->put_BrushColor1(m_pFile->GetTextColor());
			m_pRenderer->put_BrushAlpha1(255);

			// ������ ��� �����
			m_pRenderer->CommandDrawText(wsText, dX, dY, 0, 0, 0);

			if (bChangeCTM)
				m_pRenderer->ResetTransform();
		}
		void StartPath()
		{
			CheckEndPath();

			UpdateTransform();
			UpdateClip();

			m_lDrawPathType = -1;
			if (true == UpdateBrush())
			{
				unsigned int unFillMode = m_pFile->GetFillMode();
				if (ALTERNATE == unFillMode)
					m_lDrawPathType = c_nEvenOddFillMode;
				else// if (WINDING == unFillMode)
					m_lDrawPathType = c_nWindingFillMode;
			}

			if (true == UpdatePen())
			{
				if (-1 == m_lDrawPathType)
					m_lDrawPathType = c_nStroke;
				else
					m_lDrawPathType |= c_nStroke;
			}

			m_pRenderer->BeginCommand(c_nPathType);
			m_pRenderer->PathCommandStart();

			m_bStartedPath = true;
		}
		void MoveTo(int lX, int lY)
		{
			CheckStartPath(true);
			TPointD oPoint = TranslatePoint(lX, lY);
			m_pRenderer->PathCommandMoveTo(oPoint.x, oPoint.y);
		}
		void LineTo(int lX, int lY)
		{
			CheckStartPath(false);
			TPointD oPoint = TranslatePoint(lX, lY);
			m_pRenderer->PathCommandLineTo(oPoint.x, oPoint.y);
		}
		void CurveTo(int lX1, int lY1, int lX2, int lY2, int lXe, int lYe)
		{
			CheckStartPath(false);

			TPointD oPoint1 = TranslatePoint(lX1, lY1);
			TPointD oPoint2 = TranslatePoint(lX2, lY2);
			TPointD oPointE = TranslatePoint(lXe, lYe);
			m_pRenderer->PathCommandCurveTo(oPoint1.x, oPoint1.y, oPoint2.x, oPoint2.y, oPointE.x, oPointE.y);
		}
		void ArcTo(int lLeft, int lTop, int lRight, int lBottom, double dStart, double dSweep)
		{
			CheckStartPath(false);

			TPointD oTL = TranslatePoint(lLeft, lTop);
			TPointD oBR = TranslatePoint(lRight, lBottom);
			m_pRenderer->PathCommandArcTo(oTL.x, oTL.y, oBR.x - oTL.x, oBR.y - oTL.y, dStart, dSweep);
		}
		void ClosePath()
		{
			CheckStartPath(false);

			m_pRenderer->PathCommandClose();
		}
		void DrawPath(int lType = 0)
		{
			if (lType <= 0)
			{
				if (-1 != m_lDrawPathType)
					m_pRenderer->DrawPath(m_lDrawPathType);
			}
			else if (-1 != m_lDrawPathType)
			{
				bool bStroke = lType & 1 ? true : false;
				bool bFill   = lType & 2 ? true : false;

				int m_lEndType = -1;

				if (bStroke && (m_lDrawPathType & c_nStroke))
					m_lEndType = c_nStroke;

				if (bFill)
				{
					if (m_lDrawPathType & c_nWindingFillMode)
						m_lEndType = (-1 == m_lDrawPathType ? c_nWindingFillMode : m_lDrawPathType | c_nWindingFillMode);
					else if (m_lDrawPathType & c_nEvenOddFillMode)
						m_lEndType = (-1 == m_lDrawPathType ? c_nEvenOddFillMode : m_lDrawPathType | c_nEvenOddFillMode);
				}

				if (-1 != m_lEndType)
					m_pRenderer->DrawPath(m_lEndType);
			}
		}
		void EndPath()
		{
			m_pRenderer->EndCommand(c_nPathType);
			m_pRenderer->PathCommandEnd();

			m_bStartedPath = false;
		}
		void UpdateDC()
		{
			CheckEndPath();
		}
		void ResetClip()
		{
			m_pRenderer->BeginCommand(c_nResetClipType);
			m_pRenderer->EndCommand(c_nResetClipType);
		}
		void IntersectClip(int lLeft, int lTop, int lRight, int lBottom)
		{
			m_pRenderer->put_ClipMode(c_nClipRegionTypeWinding | c_nClipRegionIntersect);

			m_pRenderer->BeginCommand(c_nClipType);
			m_pRenderer->BeginCommand(c_nPathType);
			m_pRenderer->PathCommandStart();

			TPointD oTL = TranslatePoint(lLeft, lTop);
			TPointD oBR = TranslatePoint(lRight, lBottom);

			m_pRenderer->PathCommandMoveTo(oTL.x, oTL.y);
			m_pRenderer->PathCommandLineTo(oTL.x, oBR.y);
			m_pRenderer->PathCommandLineTo(oBR.x, oBR.y);
			m_pRenderer->PathCommandLineTo(oBR.x, oTL.y);
			m_pRenderer->PathCommandLineTo(oTL.x, oTL.y);

			m_pRenderer->EndCommand(c_nPathType);
			m_pRenderer->EndCommand(c_nClipType);
			m_pRenderer->PathCommandEnd();
		}
		void StartClipPath(unsigned int unMode)
		{
			CheckEndPath();

			m_bStartedPath = true;

			unsigned int unClipMode = -1;
			switch (unMode)
			{
				case RGN_AND: unClipMode = c_nClipRegionIntersect; break;
				case RGN_OR: unClipMode = c_nClipRegionUnion; break;
				case RGN_XOR: unClipMode = c_nClipRegionXor; break;
				case RGN_DIFF: unClipMode = c_nClipRegionDiff; break;
				default: unClipMode = c_nClipRegionIntersect; break;
			}

			unsigned int unFillMode = m_pFile->GetFillMode();
			if (ALTERNATE == unFillMode)
				unClipMode |= c_nClipRegionTypeEvenOdd;
			else //if (WINDING == unFillMode)
				unClipMode |= c_nClipRegionTypeWinding;

			m_pRenderer->put_ClipMode(unClipMode);
			m_pRenderer->BeginCommand(c_nClipType);
			m_pRenderer->BeginCommand(c_nPathType);
			m_pRenderer->PathCommandStart();
		}
		void EndClipPath(unsigned int unMode)
		{
			m_pRenderer->EndCommand(c_nPathType);
			m_pRenderer->EndCommand(c_nClipType);
			m_pRenderer->PathCommandEnd();

			m_bStartedPath = false;
		}

	private:

		void CheckStartPath(bool bMoveTo)
		{
			if (!m_bStartedPath)
			{
				StartPath();

				if (!bMoveTo)
				{
					TPointL oCurPos = m_pFile->GetCurPos();
					MoveTo(oCurPos.x, oCurPos.y);
				}
			}
		}
		void CheckEndPath()
		{
			if (m_bStartedPath)
			{
				DrawPath();
				EndPath();
			}
		}

		TPointD TranslatePoint(int nX, int nY)
		{
			double dX = m_pFile->TranslateX(nX);
			double dY = m_pFile->TranslateY(nY);

			// ���������� �������� ��� � ����������� ��������. ������� ������� �� �������� �� ������� ��������������, 
			// �������� ��������� ���������� � �������� �� �������� ������� ��������������.
			TRect* pBounds = m_pFile->GetDCBounds();
			double dT = pBounds->nTop;
			double dL = pBounds->nLeft;

			TEmfXForm* pInverse   = m_pFile->GetInverseTransform();
			TEmfXForm* pTransform = m_pFile->GetTransform();
			pTransform->Apply(dX, dY);
			dX -= dL;
			dY -= dT;
			pInverse->Apply(dX, dY);

			TPointD oPoint;
			oPoint.x = m_dScaleX * dX + m_dX;
			oPoint.y = m_dScaleY * dY + m_dY;
			return oPoint;
		}

		bool UpdateBrush()
		{
			IBrush* pBrush = m_pFile->GetBrush();
			if (!pBrush)
				return false;

			unsigned int unBrushStyle = pBrush->GetStyle();
			if (BS_NULL == unBrushStyle)
				return false;
			else if (BS_DIBPATTERN == unBrushStyle)
			{
				m_pRenderer->put_BrushType(c_BrushTypeTexture);
				m_pRenderer->put_BrushTextureMode(c_BrushTextureModeTile);
				m_pRenderer->put_BrushTexturePath(pBrush->GetDibPatterPath());
			}
			else //if (BS_SOLID == unBrushStyle)
			{
				m_pRenderer->put_BrushColor1(pBrush->GetColor());
				m_pRenderer->put_BrushAlpha1(pBrush->GetAlpha());
				m_pRenderer->put_BrushType(c_BrushTypeSolid);
			}

			return true;
		}
		void UpdateTransform()
		{
			double dKoefX = m_dScaleX;
			double dKoefY = m_dScaleY;

			TEmfXForm* pMatrix = m_pFile->GetTransform();
			m_pRenderer->ResetTransform();
			m_pRenderer->SetTransform(pMatrix->M11, pMatrix->M12 * dKoefY / dKoefX, pMatrix->M21 * dKoefX / dKoefY, pMatrix->M22, pMatrix->Dx * dKoefX, pMatrix->Dy * dKoefY);
		}
		bool UpdatePen()
		{
			IPen* pPen = m_pFile->GetPen();
			if (!pPen)
				return false;

			int nColor = pPen->GetColor();
			double dPixelWidth = m_pFile->GetPixelWidth();

			// TODO: dWidth ������� ��� �� ����� PS_GEOMETRIC � ����� ���������
			double dWidth = pPen->GetWidth() * m_dScaleX * dPixelWidth;
			if (dWidth <= 0.01)
				dWidth = 0;

			unsigned int unMetaPenStyle = pPen->GetStyle();
			unsigned int ulPenType   = unMetaPenStyle & PS_TYPE_MASK;
			unsigned int ulPenEndCap = unMetaPenStyle & PS_ENDCAP_MASK;
			unsigned int ulPenJoin   = unMetaPenStyle & PS_JOIN_MASK;
			unsigned int ulPenStyle  = unMetaPenStyle & PS_STYLE_MASK;

			BYTE nCapStyle = 0;
			if (PS_ENDCAP_ROUND == ulPenEndCap)
				nCapStyle = Aggplus::LineCapRound;
			else if (PS_ENDCAP_SQUARE == ulPenEndCap)
				nCapStyle = Aggplus::LineCapSquare;
			else if (PS_ENDCAP_FLAT == ulPenEndCap)
				nCapStyle = Aggplus::LineCapFlat;

			BYTE nJoinStyle = 0;
			if (PS_JOIN_ROUND == ulPenJoin)
				nJoinStyle = Aggplus::LineJoinRound;
			else if (PS_JOIN_BEVEL == ulPenJoin)
				nJoinStyle = Aggplus::LineJoinBevel;
			else if (PS_JOIN_MITER == ulPenJoin)
				nJoinStyle = Aggplus::LineJoinMiter;

			double dMiterLimit = m_pFile->GetMiterLimit() * m_dScaleX * dPixelWidth;

			// TODO: ��������� ���� ���� ���������� ����������� � ������� ����������� ���������, ������� �� ������ ��� ���� PS_SOLID.
			// TODO: ����������� PS_USERSTYLE
			BYTE nDashStyle;
			if (PS_ALTERNATE == ulPenStyle || PS_USERSTYLE == ulPenStyle || PS_INSIDEFRAME == ulPenStyle)
				nDashStyle = (BYTE)PS_SOLID;
			else if (PS_NULL != ulPenStyle)
				nDashStyle = (BYTE)ulPenStyle;

			m_pRenderer->put_PenDashStyle(nDashStyle);
			m_pRenderer->put_PenLineJoin(nJoinStyle);
			m_pRenderer->put_PenLineStartCap(nCapStyle);
			m_pRenderer->put_PenLineEndCap(nCapStyle);
			m_pRenderer->put_PenColor(nColor);
			m_pRenderer->put_PenSize(dWidth);
			m_pRenderer->put_PenAlpha(255);
			m_pRenderer->put_PenMiterLimit(dMiterLimit);

			// TO DO: � ������� ����������� AVSRenderer, ��������� ������ ushROPMode
			//        ����������� ����������. ������ ��� ������ �������� ����� ������������
			//        ��� �������� Pen'a, � ��� ��� ������ ��� ������ ����������� ��� ������.

			switch (m_pFile->GetRop2Mode())
			{
				case R2_BLACK:   m_pRenderer->put_PenColor(METAFILE_RGBA(0, 0, 0)); break;
				case R2_NOP:     m_pRenderer->put_PenAlpha(0); break;
				case R2_COPYPEN: break;
				case R2_WHITE:   m_pRenderer->put_PenColor(METAFILE_RGBA(255, 255, 255)); break;
			}

			if (PS_NULL == ulPenStyle)
				return false;

			return true;
		}
		bool UpdateClip()
		{
			IClip* pClip = m_pFile->GetClip();
			if (!pClip)
				return false;

			pClip->ClipOnRenderer(this);

			return true;
		}

	private:

		IRenderer*     m_pRenderer;
		IMetaFileBase* m_pFile;
		int            m_lDrawPathType;
		double         m_dX;      // ���������� ������ �������� ����
		double         m_dY;      //
		double         m_dW;      // 
		double         m_dH;      // 
		double         m_dScaleX; // ������������ ������/����������, ����� 
		double         m_dScaleY; // �������������� �������� ���� ������ ��������.
		bool           m_bStartedPath;
	};
}
#endif // _METAFILE_COMMON_METAFILERENDERER_H