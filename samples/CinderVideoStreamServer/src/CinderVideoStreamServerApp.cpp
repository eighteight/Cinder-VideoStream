/*
 CinderVideoStreamClientApp.cpp
 
 Created by Vladimir Gusev on 01/15/15.
 Copyright (c) 2015 onewaytheater.us
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 Based on CaptureAdvanced Cinder sample
 */
#include "cinder/app/AppNative.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Capture.h"
#include "cinder/Text.h"
#include "ConcurrentQueue.h"
#include "CinderVideoStreamServer.h"

#define USE_JPEG_COMPRESSION

using namespace ci;
using namespace ci::app;
using namespace std;

typedef CinderVideoStreamServer<uint8_t> CinderVideoStreamServerUint8;

static const int WIDTH = 1280, HEIGHT = 720;
class CinderVideoStreamServerApp : public AppNative {
 public:	
	void setup();
	void keyDown( KeyEvent event );
    void shutdown();
	void update();
	void draw();
	
 private:
	vector<Capture>		mCaptures;
	vector<gl::TextureRef>	mTextures;
	vector<gl::TextureRef>	mNameTextures;
    void threadLoop();
    bool running;
    std::string mStatus;
    
    double totalStreamSize;
    float mQuality;

    std::shared_ptr<thread> mServerThreadRef;

    ph::ConcurrentQueue<uint8_t*>* queueToServer;
};

void CinderVideoStreamServerApp::threadLoop()
{
    while (running) {
        try {
            std::shared_ptr<CinderVideoStreamServerUint8> server = std::shared_ptr<CinderVideoStreamServerUint8>(new CinderVideoStreamServerUint8(3333,queueToServer, WIDTH, HEIGHT));
            server.get()->run();
        }
        catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }
}

void CinderVideoStreamServerApp::setup()
{
	// list out the devices
    //setFrameRate(30);
	vector<Capture::DeviceRef> devices( Capture::getDevices() );
	for( vector<Capture::DeviceRef>::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt ) {
		Capture::DeviceRef device = *deviceIt;
		console() << "Found Device " << device->getName() << " ID: " << device->getUniqueId() << std::endl;
		try {
			if( device->checkAvailable() ) {
				mCaptures.push_back( Capture( WIDTH, HEIGHT, device ) );
				mCaptures.back().start();
			
				// placeholder text
				mTextures.push_back( gl::Texture::create(WIDTH, HEIGHT) );

				// render the name as a texture
				TextLayout layout;
				layout.setFont( Font( "Arial", 24 ) );
				layout.setColor( Color( 1, 1, 1 ) );
				layout.addLine( device->getName() );
				mNameTextures.push_back( gl::Texture::create( layout.render( true ) ) );
			}
			else
				console() << "device is NOT available" << std::endl;
		}
		catch( CaptureExc & ) {
			console() << "Unable to initialize device: " << device->getName() << endl;
		}
	}

    queueToServer = new ph::ConcurrentQueue<uint8_t*>();
    mServerThreadRef = std::shared_ptr<thread>(new thread(boost::bind(&CinderVideoStreamServerApp::threadLoop, this)));
    mServerThreadRef->detach();
    if (!running) running = true;
    
    totalStreamSize = 0.0;
    mQuality = 0.9f;
}

void CinderVideoStreamServerApp::shutdown(){
    running = false;
    if (queueToServer) delete queueToServer;
}

void CinderVideoStreamServerApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' )
		setFullScreen( ! isFullScreen() );
	else if( event.getChar() == ' ' ) {
		mCaptures.back().isCapturing() ? mCaptures.back().stop() : mCaptures.back().start();
	}
}

void CinderVideoStreamServerApp::update()
{

	for( vector<Capture>::iterator cIt = mCaptures.begin(); cIt != mCaptures.end(); ++cIt ) {
		if( cIt->checkNewFrame() ) {
			Surface8uRef surf = cIt->getSurface();
#ifdef USE_JPEG_COMPRESSION
            OStreamMemRef os = OStreamMem::create();
            DataTargetRef target = DataTargetStream::createRef( os );
            writeImage( target, *surf, ImageTarget::Options().quality(mQuality), "jpeg" );
            const void *data = os->getBuffer();
            size_t dataSize = os->tell();
            totalStreamSize += dataSize;

            Buffer buf( dataSize );
            memcpy(buf.getData(), data, dataSize);
            SurfaceRef jpeg = Surface::create(loadImage( DataSourceBuffer::create(buf)), SurfaceConstraintsDefault(), false );
            queueToServer->push(jpeg->getData());
            mTextures[cIt - mCaptures.begin()] = gl::Texture::create( *jpeg );
            
            mStatus.assign("Streaming JPG (")
                   .append(std::to_string((int)(mQuality*100.0f)))
                   .append("%) ")
                   .append(std::to_string((int)(totalStreamSize*0.001/getElapsedSeconds())))
                   .append(" kB/sec ")
                   .append(std::to_string((int)getFrameRate()))
                   .append(" fps ");
#else
            queueToServer->push(surf->getData());
            mTextures[cIt - mCaptures.begin()] = gl::Texture::create( *surf );
            mStatus.assign("Streaming ").append(boost::lexical_cast<std::string>(getFrameRate())).append(" fps");
#endif
            
		}
	}
}

void CinderVideoStreamServerApp::draw()
{
	gl::enableAlphaBlending();
	gl::clear( Color::black() );

	if( mCaptures.empty() )
		return;

	float width = getWindowWidth() / mCaptures.size();	
	float height = width / ( WIDTH / (float)HEIGHT );
	float x = 0, y = ( getWindowHeight() - height ) / 2.0f;
	for( vector<Capture>::iterator cIt = mCaptures.begin(); cIt != mCaptures.end(); ++cIt ) {	
		// draw the latest frame
		gl::color( Color::white() );
		if( mTextures[cIt-mCaptures.begin()] )
			gl::draw( mTextures[cIt-mCaptures.begin()], Rectf( x, y, x + width, y + height ) );
			
		// draw the name
		gl::color( Color( 0.5, 0.75, 1 ) );
		gl::draw( mNameTextures[cIt-mCaptures.begin()], vec2( x + 10, y + 10 ) );

		x += width;
	}
    
    gl::color( Color::black() );	
    gl::drawString(mStatus, vec2(10, getWindowHeight() - 10) );
}


CINDER_APP_NATIVE( CinderVideoStreamServerApp, RendererGl )
