#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include "resource.h"

#define ELLIPSE_WIDTH (8.0f)
#define WM_MONTH_PLUS (WM_APP)
#define WM_MONTH_MINUS (WM_APP+1)

TCHAR szClassName[] = TEXT("Calendar");
HWND m_hwnd;
ID2D1Factory *m_pD2DFactory;
IWICImagingFactory *m_pWICFactory;
IDWriteFactory *m_pDWriteFactory;
ID2D1HwndRenderTarget *m_pRenderTarget;
IDWriteTextFormat *m_pTitleTextFormat;
ID2D1SolidColorBrush *m_pBlackBrush;
ID2D1SolidColorBrush *m_pRedBrush;
ID2D1SolidColorBrush *m_pBlueBrush;
ID2D1SolidColorBrush *m_pGrayBrush;
ID2D1SolidColorBrush *m_pGrayRedBrush;
ID2D1SolidColorBrush *m_pGrayBlueBrush;
ID2D1Bitmap *m_pBitmap;
DWORD dwYear;
DWORD dwMonth;
DWORD dwDay;

class Date {
private:
	LANGID langID;
public:
	Date()
	{
		langID = GetUserDefaultUILanguage();
	}
	void GetCalendarName(LPWSTR lpszText) {
		switch (langID) {
		case 1033:
		case 1041: // 日本語
			lstrcpy(lpszText, L"カレンダー");
			break; 
		case 2052: // 簡体字
			lstrcpy(lpszText, L"日历");
			break;
		case 1036: // フランス語
			lstrcpy(lpszText, L"CALENDRIER");
			break;
		default: // 英語
			lstrcpy(lpszText, L"CALENDAR");
			break;
		}
	}
	void GetYearMonth(LPWSTR lpszText, DWORD dwYear, DWORD dwMonth) {
		switch (langID) {
		case 1033:
		case 1041:
		case 2052: // 簡体字
			wsprintf(lpszText, L"%d年 %d月", dwYear, dwMonth);
			break;
		case 1036: // フランス語
			{
				const LPWSTR szMonthFR[] = { L"JANVIER", L"FÉVRIER", L"MARS", L"AVRIL", L"MAI", L"JUIN", L"JUILLET", L"AOÛT", L"SEPTEMBRE", L"OCTOBRE", L"NOVEMBRE", L"DÉCEMBRE" };
				wsprintf(lpszText, L"%d %s", dwYear, szMonthFR[dwMonth - 1]);
			}
			break;
		default:
			{
				const LPWSTR szMonthEN[] = { L"January", L"February", L"March", L"April", L"May", L"June", L"July", L"August", L"September", L"October", L"November", L"December" };
				wsprintf(lpszText, L"%d %s", dwYear, szMonthEN[dwMonth - 1]);
			}
			break;
		}
	}
	void GetDayOfWeek(LPWSTR lpszText, DWORD dwIndex) {
		switch (langID) {
		case 1033:
		case 1041:
			{
				const LPWSTR szYoubiJP[] = { L"日", L"月", L"火", L"水", L"木", L"金", L"土" };
				wsprintf(lpszText, L"%s", szYoubiJP[dwIndex]);
			}
			break;
		case 2052: // 簡体字
			{
				const LPWSTR szYoubiCH[] = { L"周日", L"周一",L"周二",L"周三",L"周四",L"周五",L"周六" };
				wsprintf(lpszText, L"%s", szYoubiCH[dwIndex]);
			}
			break;
		case 1036: // フランス語
			{
				const LPWSTR szYoubiFR[] = { L"Di", L"Lu",L"Ma",L"Me",L"Je",L"Ve",L"Sa" };
				wsprintf(lpszText, L"%s", szYoubiFR[dwIndex]);
			}
			break;
		default:
			{
				const LPWSTR szYoubiEN[] = { L"Sun", L"Mon",L"Tue",L"Wed",L"Thu",L"Fri",L"Sat" };
				wsprintf(lpszText, L"%s", szYoubiEN[dwIndex]);
			}
			break;
		}
	}
};

HRESULT LoadResourceBitmap(ID2D1RenderTarget *pRenderTarget, IWICImagingFactory *pIWICFactory, PCWSTR resourceName, PCWSTR resourceType, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap **ppBitmap)
{
	IWICBitmapDecoder *pDecoder = NULL;
	IWICBitmapFrameDecode *pSource = NULL;
	IWICStream *pStream = NULL;
	IWICFormatConverter *pConverter = NULL;
	HRSRC imageResHandle = NULL;
	HGLOBAL imageResDataHandle = NULL;
	void *pImageFile = NULL;
	DWORD imageFileSize = 0;
	imageResHandle = FindResourceW(GetModuleHandle(0), resourceName, resourceType);
	HRESULT hr = imageResHandle ? S_OK : E_FAIL;
	if (SUCCEEDED(hr))
	{
		imageResDataHandle = LoadResource(GetModuleHandle(0), imageResHandle);
		hr = imageResDataHandle ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{
		pImageFile = LockResource(imageResDataHandle);
		hr = pImageFile ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{
		imageFileSize = SizeofResource(GetModuleHandle(0), imageResHandle);
		hr = imageFileSize ? S_OK : E_FAIL;
	}
	if (SUCCEEDED(hr))
	{
		hr = pIWICFactory->CreateStream(&pStream);
	}
	if (SUCCEEDED(hr))
	{
		hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(pImageFile), imageFileSize);
	}
	if (SUCCEEDED(hr))
	{
		hr = pIWICFactory->CreateDecoderFromStream(pStream, 0, WICDecodeMetadataCacheOnLoad, &pDecoder);
	}
	if (SUCCEEDED(hr))
	{
		hr = pDecoder->GetFrame(0, &pSource);
	}
	if (SUCCEEDED(hr))
	{
		hr = pIWICFactory->CreateFormatConverter(&pConverter);
	}
	if (SUCCEEDED(hr))
	{
		hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, 0, 0, WICBitmapPaletteTypeMedianCut);
	}
	if (SUCCEEDED(hr))
	{
		hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter,0,ppBitmap);
	}
	pDecoder->Release();
	pDecoder = NULL;
	pSource->Release();
	pSource = NULL;
	pStream->Release();
	pStream = NULL;
	pConverter->Release();
	pConverter = NULL;
	return hr;
}

HRESULT CreateDeviceIndependentResources()
{
	static const FLOAT msc_fontSize = 32;
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
	if (SUCCEEDED(hr))
	{
		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory), reinterpret_cast<IUnknown **>(&m_pDWriteFactory));
	}
	if (SUCCEEDED(hr))
	{
		hr = m_pDWriteFactory->CreateTextFormat(L"Meiryo", 0, DWRITE_FONT_WEIGHT_ULTRA_BLACK/*DWRITE_FONT_WEIGHT_NORMAL*/, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, msc_fontSize, L"", &m_pTitleTextFormat);
	}
	if (SUCCEEDED(hr))
	{
		m_pTitleTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		m_pTitleTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	}
	if (SUCCEEDED(hr))
	{
		CoCreateInstance(CLSID_WICImagingFactory, 0, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<void **>(&m_pWICFactory));
	}
	return hr;
}

HRESULT CreateDeviceResources()
{
	HRESULT hr = S_OK;
	if (!m_pRenderTarget)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);
		D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
		hr = m_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(m_hwnd, size), &m_pRenderTarget);
		if (SUCCEEDED(hr))
		{
			m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::ColorF(0.0f, 0.0f, 0.0f, 0.70f)), &m_pBlackBrush);
			m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::ColorF(1.0f, 0.0f, 0.0f, 0.70f)), &m_pRedBrush);
			m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::ColorF(0.0f, 0.0f, 1.0f, 0.70f)), &m_pBlueBrush);
			m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::ColorF(0.0f, 0.0f, 0.0f, 0.15f)), &m_pGrayBrush);
			m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::ColorF(1.0f, 0.0f, 0.0f, 0.15f)), &m_pGrayRedBrush);
			m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::ColorF(0.0f, 0.0f, 1.0f, 0.15f)), &m_pGrayBlueBrush);
		}
		if (SUCCEEDED(hr))
		{
			hr = LoadResourceBitmap(m_pRenderTarget, m_pWICFactory, MAKEINTRESOURCE(IDR_JPG1), L"JPG", 0, 0, &m_pBitmap);
		}
	}
	return hr;
}

void DiscardDeviceResources()
{
	m_pRenderTarget->Release();
	m_pRenderTarget = NULL;
	m_pWICFactory->Release();
	m_pWICFactory = NULL;
	m_pBlackBrush->Release();
	m_pBlackBrush = NULL;
	m_pRedBrush->Release();
	m_pRedBrush = NULL;
	m_pBlueBrush->Release();
	m_pBlueBrush = NULL;
	m_pGrayBrush->Release();
	m_pGrayBrush = NULL;
	m_pGrayRedBrush->Release();
	m_pGrayRedBrush = NULL;
	m_pGrayBlueBrush->Release();
	m_pGrayBlueBrush = NULL;
	m_pBitmap->Release();
	m_pBitmap = NULL;
}

// 指定した月の日数を返す
int NumberOfDays(int year, int month)
{
	if (month == 4 || month == 6 || month == 9 || month == 11)
		return 30;
	else if (month == 2)
	{
		if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
			return 29;
		else
			return 28;
	}
	else
		return 31;
}

// 指定した月の1日の曜日を返す(日なら0)
int GetFirstDayOfWeek(int year, int month)
{
	// 1月と2月を前年の13月と14月として扱う
	if (month <= 2)
	{
		year -= 1;
		month += 12;
	}
	// 曜日番号を返す
	return (1 + ((8 + (13 * month)) / 5) + (year + (year / 4) - (year / 100) + (year / 400))) % 7;
}

// 描画関数
HRESULT OnRender()
{
	HRESULT hr = CreateDeviceResources();
	if (SUCCEEDED(hr))
	{
		static TCHAR szText[128];
		Date date;
		date.GetYearMonth(szText, dwYear, dwMonth);
		D2D1_SIZE_F renderTargetSize = m_pRenderTarget->GetSize();
		m_pRenderTarget->BeginDraw();
		m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
		D2D1_SIZE_F size = m_pBitmap->GetSize();
		m_pRenderTarget->DrawBitmap(m_pBitmap, D2D1::RectF(0.0, 0.0, size.width, size.height), 0.4f);
		m_pRenderTarget->DrawText(szText, lstrlen(szText), m_pTitleTextFormat, D2D1::RectF(0, 0, renderTargetSize.width, renderTargetSize.height), m_pBlackBrush);
		ID2D1SolidColorBrush *pBrush;
		SYSTEMTIME systime;
		GetLocalTime(&systime);
		DWORD dwYearNow = systime.wYear;
		DWORD dwMonthNow = systime.wMonth;
		dwDay = systime.wDay;
		for (int i = 0; i < 7; i++)
		{
			date.GetDayOfWeek(szText, i);
			if (i == 6) pBrush = m_pBlueBrush;
			else if (i == 0) pBrush = m_pRedBrush;
			else pBrush = m_pBlackBrush;
			m_pRenderTarget->DrawText(szText, lstrlen(szText), m_pTitleTextFormat, D2D1::RectF((renderTargetSize.width / 7) * i, renderTargetSize.height / 8, (renderTargetSize.width / 7) * (i + 1), renderTargetSize.height / 4), pBrush);
		}
		// 現在の月の1日が何曜日か調べる
		int w = GetFirstDayOfWeek(dwYear, dwMonth);
		if (w == 0)w += 7;
		// 現在の月の日数を調べる
		const int d = NumberOfDays(dwYear, dwMonth);
		int prev;
		if (dwMonth > 1)
			prev = NumberOfDays(dwYear, dwMonth - 1);
		else
			prev = NumberOfDays(dwYear - 1, 12);
		for (int i = 0; i < w; i++)
		{
			wsprintf(szText, TEXT("%d"), prev - w + i + 1);
			if (i % 7 == 6) pBrush = m_pGrayBlueBrush;
			else if (i % 7 == 0) pBrush = m_pGrayRedBrush;
			else pBrush = m_pGrayBrush;
			m_pRenderTarget->DrawText(szText, lstrlen(szText), m_pTitleTextFormat, D2D1::RectF((renderTargetSize.width / 7.0f)*(i % 7), (renderTargetSize.height / 8.0f)*(i / 7 + 2), (renderTargetSize.width / 7.0f)*(i % 7 + 1), (renderTargetSize.height / 8.0f)*(i / 7 + 3)), pBrush);
			if ((dwYearNow == dwYear && dwMonth > 1 && dwMonthNow == dwMonth - 1 && dwDay == prev - w + i + 1) ||
				(dwYearNow == dwYear - 1 && dwMonthNow == 12 && dwMonth == 1 && dwDay == prev - w + i + 1))
			{
				m_pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F((renderTargetSize.width / 7.0f)*(i % 7) + (renderTargetSize.width / 7.0f) / 2, (renderTargetSize.height / 8.0f)*(i / 7 + 2) + (renderTargetSize.height / 8.0f) / 2.0f), (renderTargetSize.width / 7.0f) / 2.0f - ELLIPSE_WIDTH*2.0f, (renderTargetSize.height / 8.0f) / 2.0f), pBrush, ELLIPSE_WIDTH);
			}
		}
		for (int i = 1; i <= d; i++, w++)
		{
			wsprintf(szText, TEXT("%d"), i);
			if (w % 7 == 6) pBrush = m_pBlueBrush;
			else if (w % 7 == 0) pBrush = m_pRedBrush;
			else pBrush = m_pBlackBrush;
			m_pRenderTarget->DrawText(szText, lstrlen(szText), m_pTitleTextFormat, D2D1::RectF((renderTargetSize.width / 7)*(w % 7), (renderTargetSize.height / 8.0f)*(w / 7 + 2), (renderTargetSize.width / 7.0f)*(w % 7 + 1), (renderTargetSize.height / 8.0f)*(w / 7 + 3)), pBrush);
			if (dwYearNow == dwYear && dwMonthNow == dwMonth && dwDay == i)
			{
				m_pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F((renderTargetSize.width / 7.0f)*(w % 7) + (renderTargetSize.width / 7.0f) / 2.0f, (renderTargetSize.height / 8.0f)*(w / 7 + 2) + (renderTargetSize.height / 8.0f) / 2.0f), (renderTargetSize.width / 7.0f) / 2.0f - ELLIPSE_WIDTH*2.0f, (renderTargetSize.height / 8.0f) / 2.0f), pBrush, ELLIPSE_WIDTH);
			}
		}
		for (int i = 0; w / 7 < 6; i++, w++)
		{
			wsprintf(szText, TEXT("%d"), i + 1);
			if (w % 7 == 6) pBrush = m_pGrayBlueBrush;
			else if (w % 7 == 0) pBrush = m_pGrayRedBrush;
			else pBrush = m_pGrayBrush;
			m_pRenderTarget->DrawText(szText, lstrlen(szText), m_pTitleTextFormat, D2D1::RectF((renderTargetSize.width / 7)*(w % 7), (renderTargetSize.height / 8.0f)*(w / 7 + 2), (renderTargetSize.width / 7.0f)*(w % 7 + 1), (renderTargetSize.height / 8.0f)*(w / 7 + 3)), pBrush);
			if ((dwYearNow == dwYear && dwMonth < 12 && dwMonthNow == dwMonth + 1 && dwDay == i + 1) ||
				(dwYearNow == dwYear + 1 && dwMonthNow == 1 && dwMonth == 12 && dwDay == i + 1))
			{
				m_pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F((renderTargetSize.width / 7.0f)*(w % 7) + (renderTargetSize.width / 7.0f) / 2.0f, (renderTargetSize.height / 8.0f)*(w / 7 + 2) + (renderTargetSize.height / 8.0f) / 2.0f), (renderTargetSize.width / 7.0f) / 2.0f - ELLIPSE_WIDTH*2.0f, (renderTargetSize.height / 8.0f) / 2.0f), pBrush, ELLIPSE_WIDTH);
			}
		}
		hr = m_pRenderTarget->EndDraw();
		if (hr == D2DERR_RECREATE_TARGET)
		{
			hr = S_OK;
			DiscardDeviceResources();
		}
	}
	return hr;
}

void OnResize(UINT width, UINT height)
{
	if (m_pRenderTarget)
	{
		D2D1_SIZE_U size;
		size.width = width;
		size.height = height;
		m_pRenderTarget->Resize(size);
	}
}

void SetNow(HWND hWnd)
{
	SYSTEMTIME systime;
	GetLocalTime(&systime);
	dwYear = systime.wYear;
	dwMonth = systime.wMonth;
	InvalidateRect(hWnd, 0, 0);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		SetNow(hWnd);
		SetTimer(hWnd, 0x1234, 1000, 0);
		break;
	case WM_TIMER:
		{
			SYSTEMTIME systime;
			GetLocalTime(&systime);
			if (systime.wDay != dwDay)
			{
				if ((systime.wYear == dwYear && dwMonth < 12 && systime.wMonth == dwMonth + 1) ||
					(systime.wYear == dwYear + 1 && systime.wMonth == 1 && dwMonth == 12))
				{
					dwYear = systime.wYear;
					dwMonth = systime.wMonth;
				}
				InvalidateRect(hWnd, 0, 0);
			}
		}
		break;
	case WM_MONTH_PLUS:
		if (dwMonth < 12)
			dwMonth++;
		else
		{
			dwYear++;
			dwMonth = 1;
		}
		InvalidateRect(hWnd, 0, 0);
		break;
	case WM_MONTH_MINUS:
		if (dwMonth > 1)
			dwMonth--;
		else
		{
			dwYear--;
			dwMonth = 12;
		}
		InvalidateRect(hWnd, 0, 0);
		break;
	case WM_SIZE:
		OnResize(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_GETMINMAXINFO:
		{
			MINMAXINFO* lpMMI = (MINMAXINFO*)lParam;
			lpMMI->ptMaxTrackSize.x = 1024;
			lpMMI->ptMaxTrackSize.y = 768;
			return 0;
		}
	case WM_MOUSEWHEEL:
		SendMessage(hWnd, (GET_WHEEL_DELTA_WPARAM(wParam) > 0) ? WM_MONTH_MINUS : WM_MONTH_PLUS, 0, 0);
		break;
	case WM_KEYDOWN:
		{
			switch (wParam)
			{
			case VK_HOME:
				SetNow(hWnd);
				break;
			case VK_PRIOR:
			case VK_LEFT:
			case VK_UP:
				SendMessage(hWnd, WM_MONTH_MINUS, 0, 0);
				break;
			case VK_NEXT:
			case VK_RIGHT:
			case VK_DOWN:
				SendMessage(hWnd, WM_MONTH_PLUS, 0, 0);
				break;
			}
		}
		break;
	case WM_LBUTTONDOWN:
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			if (int(lParam & 0xFFFF) < rect.right / 7)
				SendMessage(hWnd, WM_MONTH_MINUS, 0, 0);
			else if (int(lParam & 0xFFFF) > 6 * rect.right / 7)
				SendMessage(hWnd, WM_MONTH_PLUS, 0, 0);
		}
		break;
	case WM_DISPLAYCHANGE:
		InvalidateRect(hWnd, 0, 0);
		break;
	case WM_PAINT:
		OnRender();
		ValidateRect(hWnd, 0);
		break;
	case WM_DESTROY:
		KillTimer(hWnd, 0x1234);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPWSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	CoInitialize(0);
	CreateDeviceIndependentResources();
	WNDCLASS wndclass = { CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance, 0, LoadCursor(0, IDC_ARROW), 0, 0, szClassName };
	RegisterClass(&wndclass);
	WCHAR szTitle[256];
	Date date;
	date.GetCalendarName(szTitle);
	m_hwnd = CreateWindow(szClassName, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, hInstance, 0);
	ShowWindow(m_hwnd, SW_SHOWDEFAULT);
	UpdateWindow(m_hwnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	m_pD2DFactory->Release();
	m_pD2DFactory = NULL;
	m_pDWriteFactory->Release();
	m_pDWriteFactory = NULL;
	m_pTitleTextFormat->Release();
	m_pTitleTextFormat = NULL;
	DiscardDeviceResources();
	CoUninitialize();
	return 0;
}
