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
	CaptureRef			mCapture;
	gl::TextureRef      mTexture;
    void threadLoop();
    bool running;
    std::string     mStatus;
    
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
	try {
		mCapture = Capture::create( WIDTH, HEIGHT );
		mCapture->start();
	}
	catch( ci::Exception &exc ) {
		console() << "Failed to initialize capture, what: " << exc.what() << std::endl;
	}

    queueToServer = new ph::ConcurrentQueue<uint8_t*>();
    mServerThreadRef = std::shared_ptr<thread>(new thread(boost::bind(&CinderVideoStreamServerApp::threadLoop, this)));
    mServerThreadRef->detach();
    if (!running) running = true;
    
    totalStreamSize = 0.0;
    mQuality = 0.1f;
}

void CinderVideoStreamServerApp::shutdown(){
    running = false;
    if (queueToServer) delete queueToServer;
}

void CinderVideoStreamServerApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' )
		setFullScreen( ! isFullScreen() );
}

void CinderVideoStreamServerApp::update()
{

    if( mCapture && mCapture->checkNewFrame() ) {
        Surface8uRef surf = mCapture->getSurface();
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
        mTexture = gl::Texture::create( *jpeg );
        
        mStatus.assign("Streaming JPG (")
               .append(std::to_string((int)(mQuality*100.0f)))
               .append("%) ")
               .append(std::to_string((int)(totalStreamSize*0.001/getElapsedSeconds())))
               .append(" kB/sec ")
               .append(std::to_string((int)getFrameRate()))
               .append(" fps ");
#else
        queueToServer->push(surf->getData());
        mTexture = gl::Texture::create( *surf );
        mStatus.assign("Streaming ").append(std::to_string((int)getFrameRate())).append(" fps");
#endif
        
    }
}

void CinderVideoStreamServerApp::draw()
{
	gl::enableAlphaBlending();
	gl::clear( Color::black() );

	if( !mCapture)
		return;

    // draw the latest frame
    gl::color( Color::white() );
    if( mTexture )
        gl::draw( mTexture, getWindowBounds() );

    gl::color( Color::black() );	
    gl::drawString(mStatus, vec2(10, getWindowHeight() - 10) );
}


CINDER_APP_NATIVE( CinderVideoStreamServerApp, RendererGl )
