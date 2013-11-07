/*
 CinderVideoStreamClientApp.cpp
 
 Created by Vladimir Gusev on 5/19/12.
 Copyright (c) 2012 onewaytheater.us
 
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

 */

#include "cinder/app/AppBasic.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "ConcurrentQueue.h"
#include "CinderVideoStreamClient.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using namespace ci;
using namespace ci::app;
using namespace std;

static const int WIDTH = 1280, HEIGHT = 720;

typedef CinderVideoStreamClient<uint8_t> CinderVideoStreamClientUint8;

class CinderVideoStreamClientApp : public AppBasic {
 public:
    void prepareSettings( Settings *settings );
	void setup();
	void keyDown( KeyEvent event );
	void update();
	void draw();
    void shutdown();
	
 private:

	gl::Texture	mTexture;

    void threadLoop();

    std::shared_ptr<std::thread> mClientThreadRef;

    uint8_t * mData;
    Surface8u mStreamSurface;

    std::string* mClientStatus;
    std::string mStatus;

    ph::ConcurrentQueue<uint8_t*>* queueFromServer;
};

void CinderVideoStreamClientApp::threadLoop()
{
    while (true) {
        try {
            boost::shared_ptr<CinderVideoStreamClientUint8> s = boost::shared_ptr<CinderVideoStreamClientUint8>(new CinderVideoStreamClientUint8("localhost","3333"));
            s.get()->setup(queueFromServer, mClientStatus, WIDTH*HEIGHT*3);  //3 - for RGB mode
            s.get()->run();
        }
        catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
        //boost::this_thread::sleep(boost::posix_time::milliseconds(1));
    }
}

void CinderVideoStreamClientApp::prepareSettings( Settings *settings )
{
	settings->setTitle("CinderVideoStreamClient");
}

void CinderVideoStreamClientApp::setup()
{	
    setFrameRate(30);
    mClientStatus = new std::string();
    queueFromServer = new ph::ConcurrentQueue<uint8_t*>();

    mClientThreadRef = std::shared_ptr<std::thread>(new std::thread(boost::bind(&CinderVideoStreamClientApp::threadLoop, this)));
    
    mStreamSurface = Surface8u(WIDTH, HEIGHT, WIDTH*3, SurfaceChannelOrder::RGB);
    mStatus.assign("Starting");
}

void CinderVideoStreamClientApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' )
		setFullScreen( ! isFullScreen() );
}

void CinderVideoStreamClientApp::shutdown(){
//    if(mClientThreadRef) {
//        mClientThreadRef->interrupt();
//        mClientThreadRef->join();
//    }
    if (queueFromServer) delete queueFromServer;
}
void CinderVideoStreamClientApp::update()
{
    if (queueFromServer->try_pop(mData)){
        memcpy(mStreamSurface.getData(), mData, WIDTH * HEIGHT * 3);
        mTexture = gl::Texture( mStreamSurface );
    }
    mStatus.assign("Client: ").append(boost::lexical_cast<std::string>(getFrameRate())).append(" fps: ").append(*mClientStatus);
}

void CinderVideoStreamClientApp::draw()
{
	gl::enableAlphaBlending();
	gl::clear( Color::black() );


	float width = getWindowWidth();
	float height = width / ( WIDTH / (float)HEIGHT );
	float x = 0, y = ( getWindowHeight() - height ) / 2.0f;

	// draw the latest frame
    gl::color( Color::white() );
	if( mTexture)
        gl::draw( mTexture, Rectf( x, y, x + width, y + height ) );
    
    // draw status
    gl::color( Color::black() );	
    gl::drawString(mStatus, Vec2f( x + 10 + 1, y + 10 + 1 ) );
}


CINDER_APP_BASIC( CinderVideoStreamClientApp, RendererGl )
