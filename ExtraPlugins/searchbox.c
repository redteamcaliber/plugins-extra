/*/*
 * Process Hacker -
 *   Subclassed Edit control
 *
 * Copyright (C) 2012-2016 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "main.h"
#include <uxtheme.h>
#include <vssym32.h>
#include <Wincodec.h>

VOID NcAreaFreeTheme(
    _Inout_ PEDIT_CONTEXT Context
    )
{
    if (Context->BrushNormal)
        DeleteObject(Context->BrushNormal);

    if (Context->BrushHot)
        DeleteObject(Context->BrushHot);

    if (Context->BrushPushed)
        DeleteObject(Context->BrushPushed);
}

VOID NcAreaInitializeFont(
    _Inout_ PEDIT_CONTEXT Context
    )
{
    LOGFONT logFont;

    if (Context->WindowFont)
        DeleteObject(Context->WindowFont);

    if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, 0))
    {
        Context->WindowFont = CreateFont(
            -PhMultiplyDivideSigned(-14, PhGlobalDpi, 72),
            0,
            0,
            0,
            FW_MEDIUM,
            FALSE,
            FALSE,
            FALSE,
            ANSI_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY | ANTIALIASED_QUALITY,
            DEFAULT_PITCH,
            logFont.lfFaceName
            );

        SendMessage(Context->WindowHandle, WM_SETFONT, (WPARAM)Context->WindowFont, TRUE);
    }
}

VOID NcAreaInitializeTheme(
    _Inout_ PEDIT_CONTEXT Context
)
{
    Context->CXWidth = PhMultiplyDivide(20, PhGlobalDpi, 96);
    Context->BrushNormal = GetSysColorBrush(COLOR_WINDOW);
    Context->BrushHot = CreateSolidBrush(RGB(205, 232, 255));
    Context->BrushPushed = CreateSolidBrush(RGB(153, 209, 255));

    if (IsThemeActive())
    {
        HTHEME themeDataHandle;

        if (themeDataHandle = OpenThemeData(Context->WindowHandle, VSCLASS_EDIT))
        {
            //IsThemePartDefined_I(themeDataHandle, EP_EDITBORDER_NOSCROLL, EPSHV_NORMAL);

            if (!SUCCEEDED(GetThemeInt(
                themeDataHandle,
                EP_EDITBORDER_NOSCROLL,
                EPSHV_NORMAL,
                TMT_BORDERSIZE,
                &Context->CXBorder
                )))
            {
                Context->CXBorder = GetSystemMetrics(SM_CXBORDER) * 2;
            }

            CloseThemeData(themeDataHandle);
        }
        else
        {
            Context->CXBorder = GetSystemMetrics(SM_CXBORDER) * 2;
        }
    }
    else
    {
        Context->CXBorder = GetSystemMetrics(SM_CXBORDER) * 2;
    }
}

VOID NcAreaInitializeImages(
    _Inout_ PEDIT_CONTEXT Context
)
{
    HBITMAP bitmap;

    Context->ImageWidth = GetSystemMetrics(SM_CXSMICON) + 4;
    Context->ImageHeight = GetSystemMetrics(SM_CYSMICON) + 4;

    if (bitmap = LoadImageFromResources(Context->ImageWidth, Context->ImageHeight, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE), TRUE))
    {
        Context->BitmapActive = BitmapToIcon(bitmap, Context->ImageWidth, Context->ImageHeight);
        DeleteObject(bitmap);
    }
    else if (bitmap = LoadImage(PluginInstance->DllBase, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_BMP), IMAGE_BITMAP, 0, 0, 0))
    {
        Context->BitmapActive = BitmapToIcon(bitmap, Context->ImageWidth, Context->ImageHeight);
        DeleteObject(bitmap);
    }

    if (bitmap = LoadImageFromResources(Context->ImageWidth, Context->ImageHeight, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE), TRUE))
    {
        Context->BitmapInactive = BitmapToIcon(bitmap, Context->ImageWidth, Context->ImageHeight);
        DeleteObject(bitmap);
    }
    else if (bitmap = LoadImage(PluginInstance->DllBase, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_BMP), IMAGE_BITMAP, 0, 0, 0))
    {
        Context->BitmapInactive = BitmapToIcon(bitmap, Context->ImageWidth, Context->ImageHeight);
        DeleteObject(bitmap);
    }
}

VOID NcAreaGetButtonRect(
    _Inout_ PEDIT_CONTEXT Context,
    _Inout_ PRECT ButtonRect
)
{
    ButtonRect->left = (ButtonRect->right - Context->CXWidth) - Context->CXBorder - 1; // offset left border by 1
    ButtonRect->bottom -= Context->CXBorder;
    ButtonRect->right -= Context->CXBorder;
    ButtonRect->top += Context->CXBorder;
}

VOID NcAreaDrawButton(
    _Inout_ PEDIT_CONTEXT Context,
    _In_ RECT ButtonRect
    )
{
    HDC hdc;
    HDC bufferDc;
    HBITMAP bufferBitmap;
    HBITMAP oldBufferBitmap;
    RECT bufferRect =
    {
        0, 0,
        ButtonRect.right - ButtonRect.left,
        ButtonRect.bottom - ButtonRect.top
    };

    if (!(hdc = GetWindowDC(Context->WindowHandle)))
        return;

    bufferDc = CreateCompatibleDC(hdc);
    bufferBitmap = CreateCompatibleBitmap(hdc, bufferRect.right, bufferRect.bottom);
    oldBufferBitmap = SelectObject(bufferDc, bufferBitmap);

    if (Context->Pushed)
    {
        FillRect(bufferDc, &bufferRect, Context->BrushPushed);
        //FrameRect(bufferDc, &bufferRect, CreateSolidBrush(RGB(0xff, 0, 0)));
    }
    else if (Context->Hot)
    {
        FillRect(bufferDc, &bufferRect, Context->BrushHot);
        //FrameRect(bufferDc, &bufferRect, CreateSolidBrush(RGB(38, 160, 218)));
    }
    else
    {
        FillRect(bufferDc, &bufferRect, Context->BrushNormal);
    }

    if (Edit_GetTextLength(Context->WindowHandle) > 0)
    {
        if (Context->BitmapActive)
        {
            DrawIconEx(
                bufferDc,
                bufferRect.left + ((bufferRect.right - bufferRect.left) - Context->ImageWidth) / 2 + 1,
                bufferRect.top + ((bufferRect.bottom - bufferRect.top) - Context->ImageHeight) / 2 + 1,
                Context->BitmapActive,
                Context->ImageWidth,
                Context->ImageHeight,
                0,
                NULL,
                DI_NORMAL
                );
        }
    }
    else
    {
        if (Context->BitmapInactive)
        {
            DrawIconEx(
                bufferDc,
                bufferRect.left + ((bufferRect.right - bufferRect.left) - (Context->ImageWidth - 2)) / 2, // offset by one
                bufferRect.top + ((bufferRect.bottom - bufferRect.top) - (Context->ImageHeight - 2)) / 2 + 1, // offset by one
                Context->BitmapInactive,
                Context->ImageWidth,
                Context->ImageHeight,
                0,
                NULL,
                DI_NORMAL
                );
        }
    }

    BitBlt(hdc, ButtonRect.left, ButtonRect.top, ButtonRect.right, ButtonRect.bottom, bufferDc, 0, 0, SRCCOPY);
    SelectObject(bufferDc, oldBufferBitmap);
    DeleteObject(bufferBitmap);
    DeleteDC(bufferDc);

    ReleaseDC(Context->WindowHandle, hdc);
}

LRESULT CALLBACK NcAreaWndSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ ULONG_PTR dwRefData
)
{
    PEDIT_CONTEXT context;

    context = (PEDIT_CONTEXT)GetProp(hWnd, L"SearchBoxContext");

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            NcAreaFreeTheme(context);

            if (context->WindowFont)
                DeleteObject(context->WindowFont);

            RemoveWindowSubclass(hWnd, NcAreaWndSubclassProc, uIdSubclass);
            RemoveProp(hWnd, L"SearchBoxContext");
            PhFree(context);
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_NCCALCSIZE:
        {
            LPNCCALCSIZE_PARAMS ncCalcSize = (NCCALCSIZE_PARAMS*)lParam;

            // Let Windows handle the non-client defaults.
            DefSubclassProc(hWnd, uMsg, wParam, lParam);

            // Deflate the client area to accommodate the custom button.
            ncCalcSize->rgrc[0].right -= context->CXWidth;
        }
        return 0;
    case WM_NCPAINT:
        {
            RECT windowRect;

            // Let Windows handle the non-client defaults.
            DefSubclassProc(hWnd, uMsg, wParam, lParam);

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Adjust the coordinates (start from 0,0).
            OffsetRect(&windowRect, -windowRect.left, -windowRect.top);

            // Get the position of the inserted button.
            NcAreaGetButtonRect(context, &windowRect);

            // Draw the button.
            NcAreaDrawButton(context, windowRect);
        }
        return 0;
    case WM_NCHITTEST:
        {
            POINT windowPoint;
            RECT windowRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Get the position of the inserted button.
            NcAreaGetButtonRect(context, &windowRect);

            // Check that the mouse is within the inserted button.
            if (PtInRect(&windowRect, windowPoint))
            {
                return HTBORDER;
            }
        }
        break;
    case WM_NCLBUTTONDOWN:
        {
            POINT windowPoint;
            RECT windowRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Get the position of the inserted button.
            NcAreaGetButtonRect(context, &windowRect);

            // Check that the mouse is within the inserted button.
            if (PtInRect(&windowRect, windowPoint))
            {
                context->Pushed = TRUE;

                SetCapture(hWnd);

                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            POINT windowPoint;
            RECT windowRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Get the position of the inserted button.
            NcAreaGetButtonRect(context, &windowRect);

            // Check that the mouse is within the inserted button.
            if (PtInRect(&windowRect, windowPoint))
            {
                // Forward click notification.
                SendMessage(GetParent(context->WindowHandle), WM_COMMAND, MAKEWPARAM(context->CommandID, BN_CLICKED), 0);
            }

            if (GetCapture() == hWnd)
            {
                context->Pushed = FALSE;
                ReleaseCapture();
            }

            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_CUT:
    case WM_CLEAR:
    case WM_PASTE:
    case WM_UNDO:
    case WM_KEYUP:
    case WM_SETTEXT:
    case WM_KILLFOCUS:
        RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        break;
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_THEMECHANGED:
        {
            NcAreaFreeTheme(context);
            NcAreaInitializeTheme(context);
            NcAreaInitializeFont(context);

            // Reset the client area margins.
            SendMessage(hWnd, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(0, 0));

            // Refresh the non-client area.
            SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

            // Force the edit control to update its non-client area.
            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_NCMOUSEMOVE:
        {
            POINT windowPoint;
            RECT windowRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Get the position of the inserted button.
            NcAreaGetButtonRect(context, &windowRect);

            // Check that the mouse is within the inserted button.
            if (PtInRect(&windowRect, windowPoint) && !context->Hot)
            {
                TRACKMOUSEEVENT trackMouseEvent;

                trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
                trackMouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT;
                trackMouseEvent.hwndTrack = hWnd;
                trackMouseEvent.dwHoverTime = 0;

                context->Hot = TRUE;

                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

                TrackMouseEvent(&trackMouseEvent);
            }
        }
        break;
    case WM_NCMOUSELEAVE:
        {
            if (context->Hot)
            {
                context->Hot = FALSE;
                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            if ((wParam & MK_LBUTTON) && GetCapture() == hWnd)
            {
                POINT windowPoint;
                RECT windowRect;

                // Get the screen coordinates of the mouse.
                if (!GetCursorPos(&windowPoint))
                    break;

                // Get the screen coordinates of the window.
                GetWindowRect(hWnd, &windowRect);

                // Get the position of the inserted button.
                NcAreaGetButtonRect(context, &windowRect);

                // Check that the mouse is within the inserted button.
                context->Pushed = PtInRect(&windowRect, windowPoint);

                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

HICON BitmapToIcon(
    _In_ HBITMAP BitmapHandle,
    _In_ INT Width,
    _In_ INT Height
    )
{
    HICON icon;
    HDC screenDc;
    HBITMAP screenBitmap;
    ICONINFO iconInfo = { 0 };

    screenDc = CreateIC(L"DISPLAY", NULL, NULL, NULL);
    screenBitmap = CreateCompatibleBitmap(screenDc, Width, Height);

    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = BitmapHandle;
    iconInfo.hbmMask = screenBitmap;

    icon = CreateIconIndirect(&iconInfo);

    DeleteObject(screenBitmap);
    DeleteDC(screenDc);

    return icon;
}

HBITMAP LoadImageFromResources(
    _In_ UINT Width,
    _In_ UINT Height,
    _In_ PCWSTR Name,
    _In_ BOOLEAN RGBAImage
    )
{
    UINT frameCount = 0;
    BOOLEAN success = FALSE;
    ULONG resourceLength = 0;
    HGLOBAL resourceHandle = NULL;
    HRSRC resourceHandleSource = NULL;
    WICInProcPointer resourceBuffer = NULL;
    HDC screenHdc = NULL;
    HDC bufferDc = NULL;
    BITMAPINFO bitmapInfo = { 0 };
    HBITMAP bitmapHandle = NULL;
    PBYTE bitmapBuffer = NULL;
    IWICStream* wicStream = NULL;
    IWICBitmapSource* wicBitmapSource = NULL;
    IWICBitmapDecoder* wicDecoder = NULL;
    IWICBitmapFrameDecode* wicFrame = NULL;
    IWICImagingFactory* wicFactory = NULL;
    IWICBitmapScaler* wicScaler = NULL;
    WICPixelFormatGUID pixelFormat;
    WICRect rect = { 0, 0, Width, Height };

    // Create the ImagingFactory
    if (FAILED(CoCreateInstance(&CLSID_WICImagingFactory1, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, &wicFactory)))
        goto CleanupExit;

    if ((resourceHandleSource = FindResource(PluginInstance->DllBase, Name, L"PNG")) == NULL)
        goto CleanupExit;

    resourceLength = SizeofResource(PluginInstance->DllBase, resourceHandleSource);

    if ((resourceHandle = LoadResource(PluginInstance->DllBase, resourceHandleSource)) == NULL)
        goto CleanupExit;

    if ((resourceBuffer = (WICInProcPointer)LockResource(resourceHandle)) == NULL)
        goto CleanupExit;

    if (FAILED(IWICImagingFactory_CreateStream(wicFactory, &wicStream)))
        goto CleanupExit;

    if (FAILED(IWICStream_InitializeFromMemory(wicStream, resourceBuffer, resourceLength)))
        goto CleanupExit;

    if (FAILED(IWICImagingFactory_CreateDecoder(wicFactory, &GUID_ContainerFormatPng, NULL, &wicDecoder)))
        goto CleanupExit;

    if (FAILED(IWICBitmapDecoder_Initialize(wicDecoder, (IStream*)wicStream, WICDecodeMetadataCacheOnLoad)))
        goto CleanupExit;

    if (FAILED(IWICBitmapDecoder_GetFrameCount(wicDecoder, &frameCount)) || frameCount < 1)
        goto CleanupExit;

    if (FAILED(IWICBitmapDecoder_GetFrame(wicDecoder, 0, &wicFrame)))
        goto CleanupExit;

    if (FAILED(IWICBitmapFrameDecode_GetPixelFormat(wicFrame, &pixelFormat)))
        goto CleanupExit;

    // Check if the image format is supported
    if (IsEqualGUID(&pixelFormat, RGBAImage ? &GUID_WICPixelFormat32bppPRGBA : &GUID_WICPixelFormat32bppPBGRA))
    {
        wicBitmapSource = (IWICBitmapSource*)wicFrame;
    }
    else
    {
        IWICFormatConverter* wicFormatConverter = NULL;

        if (FAILED(IWICImagingFactory_CreateFormatConverter(wicFactory, &wicFormatConverter)))
            goto CleanupExit;

        if (FAILED(IWICFormatConverter_Initialize(
            wicFormatConverter,
            (IWICBitmapSource*)wicFrame,
            RGBAImage ? &GUID_WICPixelFormat32bppPRGBA : &GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.0,
            WICBitmapPaletteTypeCustom
            )))
        {
            IWICFormatConverter_Release(wicFormatConverter);
            goto CleanupExit;
        }

        // Convert the image to the correct format
        IWICFormatConverter_QueryInterface(wicFormatConverter, &IID_IWICBitmapSource, &wicBitmapSource);
        IWICFormatConverter_Release(wicFormatConverter);

        // Dispose the old frame now that the converted frame is in wicBitmapSource
        IWICBitmapFrameDecode_Release(wicFrame);
    }

    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = rect.Width;
    bitmapInfo.bmiHeader.biHeight = -((LONG)rect.Height);
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    screenHdc = CreateIC(L"DISPLAY", NULL, NULL, NULL);
    bufferDc = CreateCompatibleDC(screenHdc);
    bitmapHandle = CreateDIBSection(screenHdc, &bitmapInfo, DIB_RGB_COLORS, (PVOID*)&bitmapBuffer, NULL, 0);

    // Check if it's the same rect as the requested size.
    //if (width != rect.Width || height != rect.Height)
    if (FAILED(IWICImagingFactory_CreateBitmapScaler(wicFactory, &wicScaler)))
        goto CleanupExit;
    if (FAILED(IWICBitmapScaler_Initialize(wicScaler, wicBitmapSource, rect.Width, rect.Height, WICBitmapInterpolationModeFant)))
        goto CleanupExit;
    if (FAILED(IWICBitmapScaler_CopyPixels(wicScaler, &rect, rect.Width * 4, rect.Width * rect.Height * 4, bitmapBuffer)))
        goto CleanupExit;

    success = TRUE;

CleanupExit:

    // Cleanup resources in the same order they were created.
    if (wicScaler)
    {
        IWICBitmapScaler_Release(wicScaler);
    }

    if (bufferDc)
    {
        DeleteDC(bufferDc);
    }

    if (screenHdc)
    {
        DeleteDC(screenHdc);
    }

    if (wicBitmapSource)
    {
        IWICBitmapSource_Release(wicBitmapSource);
    }

    if (wicStream)
    {
        IWICStream_Release(wicStream);
    }

    if (wicDecoder)
    {
        IWICBitmapDecoder_Release(wicDecoder);
    }

    if (wicFactory)
    {
        IWICImagingFactory_Release(wicFactory);
    }

    if (resourceHandle)
    {
        FreeResource(resourceHandle);
    }

    if (success)
        return bitmapHandle;

    DeleteObject(bitmapHandle);
    return NULL;
}

VOID CreateSearchControl(
    _In_ HWND Parent,
    _In_ HWND WindowHandle,
    _In_ UINT CommandID
    )
{
    PEDIT_CONTEXT context;

    context = (PEDIT_CONTEXT)PhAllocate(sizeof(EDIT_CONTEXT));
    memset(context, 0, sizeof(EDIT_CONTEXT));

    context->CommandID = CommandID;
    context->WindowHandle = WindowHandle;

    //NcAreaInitializeTheme(context);
    NcAreaInitializeImages(context);

    // Set initial text
    Edit_SetCueBannerText(context->WindowHandle, L"Search Plugins (Ctrl+K)");

    // Set our window context data.
    SetProp(context->WindowHandle, L"SearchBoxContext", (HANDLE)context);

    // Subclass the Edit control window procedure.
    SetWindowSubclass(context->WindowHandle, NcAreaWndSubclassProc, 0, (ULONG_PTR)context);

    // Initialize the theme parameters.
    SendMessage(context->WindowHandle, WM_THEMECHANGED, 0, 0);
}