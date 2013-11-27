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

#ifndef Rendering_h
#define Rendering_h

#include <cstddef>
#include "Wrapper_Callbacks.h"

inline void SetPixel( unsigned char* fb, int w, int h, int x, int y, int color )
{
  if ( x < 0 || y < 0 || x > w - 1 || y > h - 1 ) { return; }

  fb += ( ( h - 1 - y ) * w + x ) * 4;
  *fb++ = ( color       ) & 0xFF;
  *fb++ = ( color >> 8  ) & 0xFF;
  *fb++ = ( color >> 16 ) & 0xFF;
  *fb = 0;
}

void LineBresenham( unsigned char* fb, int w, int h, int p1x, int p1y, int p2x, int p2y, int color );

class Renderer
{
public:
  Renderer( int w, int h )
    : FWidth( w ), FHeight( h ),
      FXScale(1.f), FYScale(1.f),
      FXOfs(0.f), FYOfs(0.f),
      FFrameBuffer(NULL), FFrameBufferSize(0)
  {}

  ~Renderer();

  void SetScale( float x, float y ) { FXScale = x; FYScale = y; }
  void SetOffsets( float x, float y ) { FYOfs = x; FYOfs = y; }

  int XToScreen( float x ) { return FWidth / 2 + x * FXScale + FXOfs; }
  int YToScreen( float y ) { return FHeight / 2 - y * FYScale + FYOfs; }

  float ScreenToX( int x ) { return ( ( float )( x - FWidth / 2 )  - FXOfs ) / FXScale; }
  float ScreenToY( int y ) { return -( ( float )( y - FHeight / 2 ) - FYOfs ) / FYScale; }

  bool Init();
  
  void Clear( int color );

  inline void Line( int x1, int y1, int x2, int y2, int color )
  {
    LineBresenham( FFrameBuffer, FWidth, FHeight, x1, y1, x2, y2, color );
  }

  inline void LineW( float x1, float y1, float x2, float y2, int color )
  {
    Line( XToScreen( x1 ), YToScreen( y1 ), XToScreen( x2 ), YToScreen( y2 ), color );
  }

  const unsigned char* GetFrameBuffer() const { return FFrameBuffer; }

  size_t GetWidth() const { return FWidth; }
  size_t GetHeight() const { return FHeight; }

private:
  unsigned char* FFrameBuffer;
  size_t FFrameBufferSize;
  float FXScale, FYScale;
  float FXOfs, FYOfs;

  const int FWidth;
  const int FHeight;
};

#endif // Rendering_h
