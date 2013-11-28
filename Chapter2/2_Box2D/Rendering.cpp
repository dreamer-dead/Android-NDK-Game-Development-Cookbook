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

#include "Rendering.h"

#include <algorithm> // For std::swap
#include <cstring> // For memcpy

namespace
{
	struct LineRenderer;

	template<const bool SWAP_XY>
	void SetPixelImpl( const LineRenderer& renderer, int x, int y );

	struct LineRenderer
	{
		LineRenderer( unsigned char* buffer, int width, int height, int color )
			: FBuffer( buffer ), FWidth( width ), FHeight( height ), FColor( color )
		{}

		template <const int XDIRECTION, const int YDIRECTION, const bool COORD_COMPARE, const bool SWAP_XY>
		inline void DrawLine( const int d, const int d2, const int delta, int linearCoord, int p1y, const int linearCoordLimit ) const
		{
			int F = d2 + d;    // initial F

			while ( ( linearCoord < linearCoordLimit ) == COORD_COMPARE || linearCoord == linearCoordLimit )
			{
				SetPixelImpl<SWAP_XY>( *this, linearCoord, p1y );

				if ( F <= 0 )
				{
					F += d2;
				}
				else
				{
					p1y += YDIRECTION;
					F += delta;
				}

				linearCoord += XDIRECTION;
			}
		}

		unsigned char* FBuffer;
		const int FWidth;
		const int FHeight;
		const int FColor;
	};

	template<const bool SWAP_XY>
	void SetPixelImpl( const LineRenderer& renderer, int x, int y )
	{
		::SetPixel( renderer.FBuffer, renderer.FWidth, renderer.FHeight, x, y, renderer.FColor );
	}

	template<>
	void SetPixelImpl<true>( const LineRenderer& renderer, int x, int y )
	{
		::SetPixel( renderer.FBuffer, renderer.FWidth, renderer.FHeight, y, x, renderer.FColor );
	}
}

void LineBresenham( unsigned char* fb, int w, int h, int p1x, int p1y, int p2x, int p2y, int color )
{
	using std::swap;

	int x, y;

	if ( p1x > p2x ) // Swap points if p1 is on the right of p2
	{
		swap( p1x, p2x );
		swap( p1y, p2y );
	}

	LineRenderer lr(fb, w, h, color);
	// Handle trivial cases separately for algorithm speed up.
	// Trivial case 1: m = +/-INF (Vertical line)
	if ( p1x == p2x )
	{
		// Swap y-coordinates if p1 is above p2
		if ( p1y > p2y ) { swap( p1y, p2y ); }

		x = p1x;
		y = p1y;

		while ( y <= p2y )
		{
			SetPixelImpl< false >( lr, x, y );
			y++;
		}

		return;
	}
	// Trivial case 2: m = 0 (Horizontal line)
	else if ( p1y == p2y )
	{
		x = p1x;
		y = p1y;

		while ( x <= p2x )
		{
			SetPixelImpl< false >( lr, x, y );
			x++;
		}

		return;
	}

	const int dy            = p2y - p1y;  // y-increment from p1 to p2
	const int dx            = p2x - p1x;  // x-increment from p1 to p2
	const int dy2           = ( dy << 1 ); // dy << 1 == 2*dy
	const int dx2           = ( dx << 1 );
	const int dy2_minus_dx2 = dy2 - dx2;  // precompute constant for speed up
	const int dy2_plus_dx2  = dy2 + dx2;

	if ( dy >= 0 )  // m >= 0
	{
		// Case 1: 0 <= m <= 1 (Original case)
		if ( dy <= dx )
		{
			lr.DrawLine<1, 1, true, false>( -dx, dy2, dy2_minus_dx2, p1x, p1y, p2x );
		}
		// Case 2: 1 < m < INF (Mirror about y=x line
		// replace all dy by dx and dx by dy)
		else
		{
			lr.DrawLine<1, 1, true, true>( -dy, dx2, -dy2_minus_dx2, p1y, p1x, p2y );
		}
	}
	else    // m < 0
	{
		// Case 3: -1 <= m < 0 (Mirror about x-axis, replace all dy by -dy)
		if ( dx >= -dy )
		{
			lr.DrawLine<1, -1, true, false>( -dx, -dy2, -dy2_plus_dx2, p1x, p1y, p2x );
		}
		// Case 4: -INF < m < -1 (Mirror about x-axis and mirror
		// about y=x line, replace all dx by -dy and dy by dx)
		else
		{
			lr.DrawLine<-1, 1, false, true>( dy, dx2, dy2_plus_dx2, p1y, p1x, p2y );
		}
	}
}

bool Renderer::Init()
{
	FFrameBuffer.resize( FWidth * FHeight * 4 );

	// TODO: Use std::fill() instead memset.
	memset( &FFrameBuffer[0], 0xFF, FFrameBuffer.size() );
}

void Renderer::Clear( int color ) const
{
	const unsigned char PATTERN[] = { color & 0xFF, ( color >> 8 ) & 0xFF, ( color >> 16 ) & 0xFF, 0x00 };
	size_t BlockSize = sizeof( PATTERN ); // Size of pattern

	if ( FFrameBuffer.size() < BlockSize )
		return;

	unsigned char* Ptr = &FFrameBuffer[0];
	memcpy( Ptr, PATTERN, BlockSize );

	unsigned char * Start = Ptr;
	unsigned char * Current = Ptr + BlockSize;
	unsigned char * End = Start + FFrameBuffer.size();

	// Fill the buffer with pattern.
	while( Current + BlockSize < End ) {
	    memcpy( Current, Start, BlockSize );
	    Current += BlockSize;
	    BlockSize *= 2;
	}
	// Fill the rest.
	memcpy( Current, Start, static_cast<size_t>( End - Current ) );
}
