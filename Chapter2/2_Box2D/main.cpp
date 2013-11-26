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

#include <windows.h>

typedef LARGE_INTEGER time_value_t;

#define TIME_TO_DOUBLE(t) static_cast<double>( t.QuadPart )

#endif // _WIN32
	
namespace
{
	const float TIME_STEP = 0.016666f;

	using namespace Box2D;

	void DrawLine2( const Vec2& v1, const Vec2& v2 )
	{
		LineW( v1.x, v1.y, v2.x, v2.y, 0 ); // 0xFFFFFF);
	}

	void DrawBody( const Body& body )
	{
		const Mat22& R = body.rotation;
		const Vec2& x = body.position;
		const Vec2& h = 0.5f * body.width;

		Vec2 v[] = { ( x + R * Vec2( -h.x, -h.y ) ), ( x + R * Vec2( h.x, -h.y ) ), ( x + R * Vec2( h.x,  h.y ) ), ( x + R * Vec2( -h.x,  h.y ) ) };

		for ( int i = 0 ; i < 4 ; i++ ) { DrawLine2( v[i], v[( i + 1 ) % 4] ); };
	}

	void DrawJoint( const Joint& joint )
	{
		Body* b1 = joint.body1;
		Body* b2 = joint.body2;

		const Mat22& R1 = b1->rotation;
		const Mat22& R2 = b2->rotation;

		const Vec2& x1 = b1->position;
		const Vec2& p1 = x1 + R1 * joint.localAnchor1;

		const Vec2& x2 = b2->position;
		const Vec2& p2 = x2 + R2 * joint.localAnchor2;

		DrawLine2( x1, p1 );
		DrawLine2( p1, x2 );
		DrawLine2( x2, p2 );
		DrawLine2( p2, x1 );
	}

	struct Box2DEventObserver : public EventObserver
	{
		Box2DEventObserver()
			: FLocalTime(0.f)
			  , FRecipCyclesPerSecond(1.f)
		{}

		virtual ~Box2DEventObserver() {}

		virtual void OnStart();
		virtual void OnDrawFrame();
		virtual void OnTimer( float Delta );

	private:
		void StartTiming();
		double GetSeconds();
		void GenerateTicks();

		double FNewTime, FOldTime, FExecutionTime;
		float FLocalTime;
		float FRecipCyclesPerSecond;
		std::auto_ptr<World> FWorld;
	};

	void Box2DEventObserver::OnStart()
	{
		XScale = YScale = 15.0;
		XOfs = YOfs = 0;

		const size_t FrameBufferSize = ImageWidth * ImageHeight * 4;
		g_FrameBuffer = ( unsigned char* )malloc( FrameBufferSize );
		memset( g_FrameBuffer, 0xFF, FrameBufferSize );

		StartTiming();

		FOldTime = GetSeconds();
		FNewTime = FOldTime;

		FExecutionTime = 0;

		FWorld.reset( new World( Vec2( 0, 0 ), 10 ) );
		setup3( FWorld.get() );
	}

	void Box2DEventObserver::OnDrawFrame()
	{
		// render physics world
		Clear( 0xFFFFFF );

		std::vector<Body*>::const_iterator BodyBegin = FWorld->bodies.begin(), BodyEnd = FWorld->bodies.end();
		for ( ; BodyBegin != BodyEnd ; ++BodyBegin )
		{
			DrawBody( **BodyBegin );
		}

		std::vector<Joint*>::const_iterator JointsBegin = FWorld->joints.begin(), JointsEnd = FWorld->joints.end();
		for ( ; JointsBegin != JointsEnd ; ++JointsBegin )
		{
			DrawJoint( **JointsBegin );
		}

		// update as fast as possible
		GenerateTicks();
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
		FNewTime = GetSeconds();
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

	void Box2DEventObserver::StartTiming()
	{
		/// Initialize timing
		time_value_t Freq;

		QueryPerformanceFrequency( &Freq );

		FRecipCyclesPerSecond = 1.0 / TIME_TO_DOUBLE(Freq);
	}

	double Box2DEventObserver::GetSeconds()
	{
		time_value_t Counter;

		QueryPerformanceCounter( &Counter );

		return FRecipCyclesPerSecond * TIME_TO_DOUBLE(Counter);
	}
} // namespace

// static
EventObserver* PlatformLayer::CreateObserver(PlatformLayer* platform)
{
	static Box2DEventObserver observer;

	return &observer;
}
