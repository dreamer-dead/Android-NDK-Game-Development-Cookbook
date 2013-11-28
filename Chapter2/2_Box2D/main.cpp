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

#include <stdlib.h>
#include <memory>
#include <algorithm>

#include "Wrapper_Callbacks.h"
#include "Rendering.h"
#include "BoxLite.h"
#include "BoxSample.h"

#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <pthread.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

namespace 
{
	const unsigned usec_per_sec = 1000000;

	bool QueryPerformanceFrequency( time_value_t* frequency )
	{
		/* gettimeofday reports to microsecond accuracy. */
		*frequency = usec_per_sec;
		return true;
	}

	bool QueryPerformanceCounter( time_value_t* performance_count )
	{
		struct timeval Time;

		gettimeofday( &Time, NULL );
		*performance_count = Time.tv_usec + Time.tv_sec * usec_per_sec;

		return true;
	}
} // namespace

typedef int64_t time_value_t;

#define TIME_TO_DOUBLE(t) static_cast<double>( t )

#else

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef LARGE_INTEGER time_value_t;

#define TIME_TO_DOUBLE(t) static_cast<double>( t.QuadPart )

#endif // ifndef _WIN32
	
namespace
{
	static const float TIME_STEP = 0.016666f;
	static const int ImageWidth = 512;
	static const int ImageHeight = 512;

	using namespace Box2D;

	// TODO: Move Timer class to it's own header + impl
	struct Timer
	{
		Timer() : FRecipCyclesPerSecond( 1.f ) {}

		void StartTiming();
		double GetSeconds() const;

	private:
		double FRecipCyclesPerSecond;
	};

	void Timer::StartTiming()
	{
		/// Initialize timing
		time_value_t Freq;

		QueryPerformanceFrequency( &Freq );

		FRecipCyclesPerSecond = 1.0 / TIME_TO_DOUBLE(Freq);
	}

	double Timer::GetSeconds() const
	{
		time_value_t Counter;

		QueryPerformanceCounter( &Counter );

		return FRecipCyclesPerSecond * TIME_TO_DOUBLE(Counter);
	}

	struct Box2DEventObserver: public EventObserver
	{
		Box2DEventObserver()
			: FLocalTime(0.f),
				FRenderer(ImageWidth, ImageHeight)
		{}

		virtual ~Box2DEventObserver() {}

		virtual void OnStart();
		virtual void OnDrawFrame( DrawFrameInfo* frameInfo );
		virtual void OnTimer( float Delta );

	private:
		struct BoxObjectsRenderer
		{
			BoxObjectsRenderer( int w, int h )
				: FRenderer( w, h ),
					FCurrentColor( 0 ),
					FFillColor( 0xFFFFFF )
			{}

			bool Init(const Vec2& scale, const Vec2& offset)
			{
				FRenderer.SetScale( scale.x, scale.y );
				FRenderer.SetOffsets( offset.x, offset.y );
				return FRenderer.Init();
			}

			void SetCurrentColor( int color )
			{
				FCurrentColor = color;
			}

			void DrawLine( const Vec2& v1, const Vec2& v2 ) const
			{
				FRenderer.LineW( v1.x, v1.y, v2.x, v2.y, FCurrentColor );
			}

			void Draw( const Body& body ) const;
			void Draw( const Joint& joint ) const;

			void Clear() const
			{
				FRenderer.Clear( FFillColor );
			}

			void FillDrawFrameInfo( DrawFrameInfo* frameInfo ) const
			{
				frameInfo->frame = FRenderer.GetFrameBuffer();
				frameInfo->frameWidth = FRenderer.GetWidth();
				frameInfo->frameHeight = FRenderer.GetHeight();
			}

			Renderer FRenderer;
			int FCurrentColor;
			int FFillColor;
		};

		struct RenderBoxObjectFunc
		{
			RenderBoxObject( BoxObjectsRenderer& renderer )
				: FRenderer( renderer )
			{}

			template<typename BoxObjectType>
			void operator ()( const BoxObjectType* object ) const
			{
				FRenderer.Draw( *object );
			}

			BoxObjectsRenderer& FRenderer;
		};

		void GenerateTicks();

		double FNewTime, FOldTime, FExecutionTime;
		float FLocalTime;
		float FRecipCyclesPerSecond;
		std::auto_ptr<World> FWorld;
		BoxObjectsRenderer FRenderer;
		Timer FTimer;
	};

	void Box2DEventObserver::BoxObjectsRenderer::Draw( const Body& body ) const
	{
		const Mat22& R = body.rotation;
		const Vec2& x = body.position;
		const Vec2& h = 0.5f * body.width;

		Vec2 v[] = { ( x + R * Vec2( -h.x, -h.y ) ), ( x + R * Vec2( h.x, -h.y ) ), ( x + R * Vec2( h.x,  h.y ) ), ( x + R * Vec2( -h.x,  h.y ) ) };

		for ( int i = 0 ; i < 4 ; i++ ) { DrawLine( v[i], v[( i + 1 ) % 4] ); };
	}

	void Box2DEventObserver::BoxObjectsRenderer::Draw( const Joint& joint ) const
	{
		Body* b1 = joint.body1;
		Body* b2 = joint.body2;

		const Mat22& R1 = b1->rotation;
		const Mat22& R2 = b2->rotation;

		const Vec2& x1 = b1->position;
		const Vec2& p1 = x1 + R1 * joint.localAnchor1;

		const Vec2& x2 = b2->position;
		const Vec2& p2 = x2 + R2 * joint.localAnchor2;

		DrawLine( x1, p1 );
		DrawLine( p1, x2 );
		DrawLine( x2, p2 );
		DrawLine( p2, x1 );
	}

	void Box2DEventObserver::OnStart()
	{
		// TODO: Handling result from Init call.
		FRenderer.Init( Vec2( 15.f, 15.f ), Vec2( 0.f, 0.f ));

		FTimer.StartTiming();

		FOldTime = FTimer.GetSeconds();
		FNewTime = FOldTime;

		FExecutionTime = 0;

		FWorld.reset( new World( Vec2( 0, 0 ), 10 ) );
		setup3( FWorld.get() );
	}

	void Box2DEventObserver::OnDrawFrame( DrawFrameInfo* frameInfo )
	{
		// render physics world
		FRenderer.Clear();

		const RenderBoxObjectFunc RenderFunc(FRenderer);
		std::for_each( FWorld->bodies.begin(), FWorld->bodies.end(), RenderFunc );
		std::for_each( FWorld->joints.begin(), FWorld->joints.end(), RenderFunc );

		// update as fast as possible
		GenerateTicks();

		FRenderer.FillDrawFrameInfo( frameInfo );
	}

	void Box2DEventObserver::OnTimer( float Delta )
	{
		FWorld->Step( Delta );
	}

	void Box2DEventObserver::GenerateTicks()
	{
		static const float TIME_QUANTUM = 0.0166666f;
		static const float MAX_EXECUTION_TIME = 10.0f * TIME_QUANTUM;

		// update time
		FNewTime = FTimer.GetSeconds();
		float DeltaSeconds = static_cast<float>( FNewTime - FOldTime );
		FOldTime = FNewTime;

		FExecutionTime += DeltaSeconds;

		/// execution limit
		if ( FExecutionTime > MAX_EXECUTION_TIME ) { FExecutionTime = MAX_EXECUTION_TIME; }

		/// Generate internal time quanta
		while ( FExecutionTime > TIME_QUANTUM )
		{
			FExecutionTime -= TIME_QUANTUM;
			OnTimer( TIME_QUANTUM );
		}
	}
} // namespace

// static
EventObserver* PlatformLayer::CreateObserver(PlatformLayer* platform)
{
	return new Box2DEventObserver();
}
