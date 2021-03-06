/*
* Copyright (c) 2006-2007 Erin Catto http://www.box2d.org
* Copyright (c) 2013 Google, Inc.
* Copyright (c) 2015 The Cinder Project
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "Render.h"
//
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/Surface.h"
#include "cinder/TriMesh.h"
using namespace ci;

#include <stdio.h>
#include <stdarg.h>

// Global vars for drawing in Cinder
static int gWindowWidth = 0;
static int gWindowHeight = 0;
static CameraOrtho gOrthoCam;
static gl::TextureFontRef gFont18;
static gl::TextureRef gParticleTex;

const float kPointSize = 0.05f/40.0f;
static float gPointScale = kPointSize;

void LoadOrtho2DMatrix( int windowWidth, int windowHeight, double left, double right, double bottom, double top)
{
	gWindowWidth = windowWidth;
	gWindowHeight = windowHeight;

	gOrthoCam.setOrtho( left, right, bottom, top, -1.0f, 1.0f );

	gPointScale = kPointSize*(top - bottom);
}

void DebugDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	gl::setMatrices( gOrthoCam );

	gl::color( color.r, color.g, color.b );
	gl::begin( GL_LINE_LOOP );
	for( int32 i = 0; i < vertexCount; ++i ) {
		gl::vertex( vec2( vertices[i].x, vertices[i].y ) );
	}
	gl::end();
}

void DebugDraw::DrawFlatPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	gl::setMatrices( gOrthoCam );

	gl::color( color.r, color.g, color.b, 1.0f );
	gl::begin( GL_TRIANGLE_FAN );
	for( int32 i = 0; i < vertexCount; ++i ) {
		gl::vertex( vec2( vertices[i].x, vertices[i].y ) );
	}
	gl::end();
}

void DebugDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	gl::setMatrices( gOrthoCam );

	{
		gl::ScopedBlendAlpha scopedBlend();

		gl::color( 0.5f * color.r, 0.5f * color.g, 0.5f * color.b, 0.5f );
		gl::begin( GL_TRIANGLE_FAN );
		for( int32 i = 0; i < vertexCount; ++i ) {
			gl::vertex( vec2( vertices[i].x, vertices[i].y ) );
		}
		gl::end();
	}

	gl::color( color.r, color.g, color.b, 1.0f );
	gl::begin( GL_LINE_LOOP );
	for( int32 i = 0; i < vertexCount; ++i ) {
		gl::vertex( vec2( vertices[i].x, vertices[i].y ) );
	}
	gl::end();
}

void DebugDraw::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color)
{
	gl::setMatrices( gOrthoCam );

	gl::color( color.r, color.g, color.b );
	gl::drawStrokedCircle( vec2( center.x, center.y ), radius, 16 );
}

float smoothstep(float x) { return x * x * (3 - 2 * x); }

void DebugDraw::DrawParticles(const b2Vec2 *centers, float32 radius, const b2ParticleColor *colors, int32 count)
{
	if( ! gParticleTex ) {
		const int TSIZE = 64;
		unsigned char tex[TSIZE][TSIZE][4];
		for (int y = 0; y < TSIZE; y++)
		{
			for (int x = 0; x < TSIZE; x++)
			{
				float fx = (x + 0.5f) / TSIZE * 2 - 1;
				float fy = (y + 0.5f) / TSIZE * 2 - 1;
				float dist = sqrtf(fx * fx + fy * fy);
				unsigned char intensity = (unsigned char)(dist <= 1 ? smoothstep(1 - dist) * 255 : 0);
				tex[y][x][0] = tex[y][x][1] = tex[y][x][2] = 128;
				tex[y][x][3] = intensity;
			}
		}

		SurfaceRef surf = Surface::create( (uint8_t*)tex, TSIZE, TSIZE, TSIZE*4, SurfaceChannelOrder::RGBA );
		gParticleTex = gl::Texture::create( *surf );
	}

	const float particle_size_multiplier = 2.0f;  // because of falloff
	const float pointSize = radius  * particle_size_multiplier;

	// Not the fastest way to draw particles
	TriMesh mesh( TriMesh::Format().positions( 2 ).colors( 4 ).texCoords() );
	for( int32 i = 0; i < count; ++i ) {
		// Counter-clockwise
		vec2 P0 = vec2( centers[i].x, centers[i].y ) + vec2( -pointSize, -pointSize );
		vec2 P1 = vec2( centers[i].x, centers[i].y ) + vec2(  pointSize, -pointSize );
		vec2 P2 = vec2( centers[i].x, centers[i].y ) + vec2(  pointSize,  pointSize );
		vec2 P3 = vec2( centers[i].x, centers[i].y ) + vec2( -pointSize,  pointSize );
		vec2 uv0 = vec2( 0, 0 );
		vec2 uv1 = vec2( 1, 0 );
		vec2 uv2 = vec2( 1, 1 );
		vec2 uv3 = vec2( 0, 1 );
		ColorA c = ColorA( 1.0f, 1.0f, 1.0f, 1.0f );
		if( nullptr != colors ) {
			c = ColorA( colors[i].r/255.0f, colors[i].g/255.0f, colors[i].b/255.0f, colors[i].a/255.0f );
		}
		mesh.appendPosition( P0 );
		mesh.appendPosition( P1 );
		mesh.appendPosition( P2 );
		mesh.appendPosition( P3 );
		mesh.appendTexCoord0( uv0 );
		mesh.appendTexCoord0( uv1 );
		mesh.appendTexCoord0( uv2 );
		mesh.appendTexCoord0( uv3 );
		mesh.appendColorRgba( c );
		mesh.appendColorRgba( c );
		mesh.appendColorRgba( c );
		mesh.appendColorRgba( c );
		size_t n = mesh.getNumVertices();
		uint32_t v0 = n - 4;
		uint32_t v1 = n - 3;
		uint32_t v2 = n - 2;
		uint32_t v3 = n - 1;
		mesh.appendTriangle( v0, v1, v2 );
		mesh.appendTriangle( v0, v2, v3 );
	}

	gl::setMatrices( gOrthoCam );

	gl::ScopedBlendAlpha scopedBlend();

	gl::GlslProgRef shader = gl::context()->getStockShader( gl::ShaderDef().texture( gParticleTex ).color() );
	gl::ScopedGlslProg scopedShader( shader );

	gl::ScopedTextureBind scopedTex( gParticleTex, 0 );

	gl::draw( mesh );
}

void DebugDraw::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
{
	gl::setMatrices( gOrthoCam );

	gl::ScopedBlendAlpha scopedBlend();

	gl::color( 0.5f * color.r, 0.5f * color.g, 0.5f * color.b, 0.5f );
	gl::drawSolidCircle( vec2( center.x, center.y ), radius, 16 );

	gl::color( color.r, color.g, color.b, 1.0f );
	gl::drawStrokedCircle( vec2( center.x, center.y ), radius, 16 );

	b2Vec2 p = center + radius * axis;
	gl::drawLine( vec2( center.x, center.y ), vec2( p.x, p.y ) );
}

void DebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
	gl::setMatrices( gOrthoCam );

	gl::color( color.r, color.g, color.b );
	gl::drawLine( vec2( p1.x, p1.y ), vec2( p2.x, p2.y ) );
}

void DebugDraw::DrawTransform(const b2Transform& xf)
{
	gl::setMatrices( gOrthoCam );

	b2Vec2 p1 = xf.p, p2;
	const float32 k_axisScale = 0.4f;

	gl::color( 1.0f, 0.0f, 0.0f );
	p2 = p1 + k_axisScale * xf.q.GetXAxis();
	gl::drawLine( vec2( p1.x, p1.y ), vec2( p2.x, p2.y ) );

	gl::color( 0.0f, 1.0f, 0.0f );
	p2 = p1 + k_axisScale * xf.q.GetYAxis();
	gl::drawLine( vec2( p1.x, p1.y ), vec2( p2.x, p2.y ) );
}

void DebugDraw::DrawPoint(const b2Vec2& p, float32 size, const b2Color& color)
{
	gl::setMatrices( gOrthoCam );

	gl::color( color.r, color.g, color.b );

	size *= gPointScale;
	Rectf r = Rectf( p.x - size, p.y - size, p.x + size, p.y + size );
	gl::drawSolidRect( r );
}

void DebugDraw::DrawString(int x, int y, const char *string, ...)
{
	if( ! gFont18 ) {
		gFont18 = gl::TextureFont::create( ci::Font( "Arial", 18.0f ) );
	}

	char buffer[128];

	va_list arg;
	va_start(arg, string);
	vsprintf(buffer, string, arg);
	va_end(arg);

	std::string s = std::string( buffer, strlen( buffer ) );

	// Use a pixel aligned camera with origin at top left
	gl::setMatricesWindow( gWindowWidth, gWindowHeight, true );

	gl::color( 0.9f, 0.6f, 0.6f );
	// Cinder draws using baseline so offset on y-axis
	gFont18->drawString( s, vec2( x, y + 18.0f ) );
}

void DebugDraw::DrawString(const b2Vec2& p, const char *string, ...)
{
/*
#if !defined(__ANDROID__) && !defined(__IOS__)
	char buffer[128];

	va_list arg;
	va_start(arg, string);
	vsprintf(buffer, string, arg);
	va_end(arg);

	glColor3f(0.5f, 0.9f, 0.5f);
	glRasterPos2f(p.x, p.y);

	int32 length = (int32)strlen(buffer);
	for (int32 i = 0; i < length; ++i)
	{
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, buffer[i]);
	}

	glPopMatrix();
#endif // !defined(__ANDROID__) && !defined(__IOS__)
*/
}

void DebugDraw::DrawAABB(b2AABB* aabb, const b2Color& c)
{
	gl::color( c.r, c.g, c.b );

	Rectf r = Rectf( aabb->lowerBound.x, aabb->lowerBound.y, aabb->upperBound.x, aabb->upperBound.y );
	gl::drawStrokedRect( r );
}

float ComputeFPS()
{
/*
	static bool debugPrintFrameTime = false;
	static int lastms = 0;
	int curms = glutGet(GLUT_ELAPSED_TIME);
	int delta = curms - lastms;
	lastms = curms;

	static float dsmooth = 16;
	dsmooth = (dsmooth * 30 + delta) / 31;

	if ( debugPrintFrameTime )
	{
#ifdef ANDROID
		__android_log_print(ANDROID_LOG_VERBOSE, "Testbed", "msec = %f", dsmooth);
#endif
	}

	return dsmooth;
*/
	return 0.0f;
}
