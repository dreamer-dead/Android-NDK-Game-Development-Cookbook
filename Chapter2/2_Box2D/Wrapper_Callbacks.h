/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must display the names 'Sergey Kosarevsky' and
 *    'Viktor Latypov'in the credits of the application, if such credits exist.
 *    The authors of this work must be notified via email (sk@linderdaum.com) in
 *    this case of redistribution.
 *
 * 3. Neither the name of copyright holders nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Wrapper_Callbacks_h
#define Wrapper_Callbacks_h

struct DrawFrameInfo
{
	const unsigned char* frame;
	int frameWidth;
	int frameHeight;
};

struct EventObserver
{
	virtual void OnStart() {}
	virtual void OnDrawFrame( DrawFrameInfo* frameInfo ) {}
	virtual void OnTimer( float Delta ) {}
	virtual void OnKeyUp( int code ) {}
	virtual void OnKeyDown( int code ) {}
	virtual void OnMouseDown( int btn, int x, int y ) {}
	virtual void OnMouseMove( int x, int y ) {}
	virtual void OnMouseUp( int btn, int x, int y ) {}

	virtual ~EventObserver() {}
};

struct PlatformLayer
{
	static EventObserver* CreateObserver(PlatformLayer* platform);

	virtual ~PlatformLayer() {}

protected:
	void FireOnStart() { if (FObserver) FObserver->OnStart(); }
	void FireOnDrawFrame( DrawFrameInfo* frameInfo ) { if (FObserver) FObserver->OnDrawFrame(frameInfo); }
	void FireOnTimer( float Delta ) { if (FObserver) FObserver->OnTimer(Delta); }
	void FireOnKeyUp( int code ) { if (FObserver) FObserver->OnKeyUp(code); }
	void FireOnKeyDown( int code ) { if (FObserver) FObserver->OnKeyDown(code); }
	void FireOnMouseDown( int btn, int x, int y ) { if (FObserver) FObserver->OnMouseDown(btn, x, y); }
	void FireOnMouseMove( int x, int y ) { if (FObserver) FObserver->OnMouseMove(x, y); }
	void FireOnMouseUp( int btn, int x, int y ) { if (FObserver) FObserver->OnMouseUp(btn, x, y); }

	EventObserver* FObserver;
};

#endif // Wrapper_Callbacks_h
