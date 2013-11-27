#include "Wrapper_Windows.h"

namespace
{
	static struct
	{
		HWND window;
		WindowsPlatformLayer* object;
	} g_windowToClassMap;
}  // namespace

// static
LRESULT CALLBACK WindowsPlatformLayer::WindowProcedureThunk( HWND h, UINT msg, WPARAM w, LPARAM p )
{
	if ( g_windowToClassMap.window != h || !g_windowToClassMap.object )
		return DefWindowProc( h, msg, w, p );

	return g_windowToClassMap.object->WindowProcedure( h, msg, w, p );
}

WindowsPlatformLayer::WindowsPlatformLayer()
{
	FObserver = CreateObserver( this );
}

WindowsPlatformLayer::~WindowsPlatformLayer()
{
	DeleteDC( FMemDC );
	DeleteObject( FBufferBitmap );
}

bool WindowsPlatformLayer::Init( const char* className, const char* windowTitle )
{
	FireOnStart();

	WNDCLASS wndClass;
	memset( &wndClass, 0, sizeof( WNDCLASS ) );
	wndClass.lpszClassName = className;
	wndClass.lpfnWndProc = &WindowsPlatformLayer::WindowProcedureThunk;
	wndClass.hCursor = LoadCursor( NULL, IDC_ARROW );

	if ( !RegisterClass( &wndClass ) ) { return false; }

	DrawFrameInfo frameInfo;
	FireOnDrawFrame( &frameInfo );

	RECT Rect;
	Rect.left = 0;
	Rect.top = 0;
	Rect.right  = frameInfo.frameWidth;
	Rect.bottom = frameInfo.frameHeight;

	DWORD dwStyle = WS_OVERLAPPEDWINDOW;

	AdjustWindowRect( &Rect, dwStyle, false );

	int WinWidth  = Rect.right  - Rect.left;
	int WinHeight = Rect.bottom - Rect.top;

	FWindowHandle = CreateWindowA( className, windowTitle, dwStyle, 100, 100, WinWidth, WinHeight, 0, NULL, NULL, NULL );
	g_windowToClassMap.window = FWindowHandle;
	g_windowToClassMap.object = this;

	ShowWindow( FWindowHandle, SW_SHOW );

	HDC dc = GetDC( FWindowHandle );

	// Create the offscreen device context and buffer
	FMemDC = CreateCompatibleDC( dc );
	FBufferBitmap = CreateCompatibleBitmap( dc, frameInfo.frameWidth, frameInfo.frameHeight );

	// filling the RGB555 bitmap header
	memset( &FBitmapInfo.bmiHeader, 0, sizeof( BITMAPINFOHEADER ) );
	FBitmapInfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	FBitmapInfo.bmiHeader.biWidth = frameInfo.frameWidth;
	FBitmapInfo.bmiHeader.biHeight = frameInfo.frameHeight;
	FBitmapInfo.bmiHeader.biPlanes = 1;
	FBitmapInfo.bmiHeader.biBitCount = 32;
	FBitmapInfo.bmiHeader.biSizeImage = frameInfo.frameWidth * frameInfo.frameHeight * 4;

	UpdateWindow( FWindowHandle );

	SetTimer( FWindowHandle, 1, 10, NULL );
}

LRESULT WindowsPlatformLayer::WindowProcedure( HWND h, UINT msg, WPARAM w, LPARAM p )
{
	HDC dc;
	PAINTSTRUCT ps;
	DrawFrameInfo frameInfo;
	int x = ( ( int )( short )LOWORD( p ) ), y = ( ( int )( short )HIWORD( p ) );

	switch ( msg )
	{
		case WM_KEYUP:
			FireOnKeyUp( w );
			break;

		case WM_KEYDOWN:
			FireOnKeyDown( w );
			break;

		case WM_LBUTTONDOWN:
			SetCapture( h );
			FireOnMouseDown( 0, x, y );
			break;

		case WM_MOUSEMOVE:
			FireOnMouseMove( x, y );
			break;

		case WM_LBUTTONUP:
			FireOnMouseUp( 0, x, y );
			ReleaseCapture();
			break;

		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;

		case WM_PAINT:
			FireOnDrawFrame( &frameInfo );
			dc = BeginPaint( h, &ps );
			// transfer the g_FrameBuffer to the FBufferBitmap
			SetDIBits( FMemDC, FBufferBitmap, 0, frameInfo.frameHeight, frameInfo.frame, &FBitmapInfo, DIB_RGB_COLORS );
			SelectObject( FMemDC, FBufferBitmap );
			// Copying the offscreen buffer to the window surface
			BitBlt( dc, 0, 0, frameInfo.frameWidth, frameInfo.frameHeight, FMemDC, 0, 0, SRCCOPY );
			EndPaint( h, &ps );
			break;

		case WM_TIMER:
			InvalidateRect( h, NULL, 1 );
			break;
	}

	return DefWindowProc( h, msg, w, p );
}

int main()
{
	g_windowToClassMap.window = NULL;
	g_windowToClassMap.object = NULL;
	WindowsPlatformLayer platform;

	if ( !platform.Init( "MyWinClass", "App4" ) ) { return 0; }

	MSG msg;
	while ( GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	return msg.wParam;
}
