/*
 _TBOX_PREFIX_App.cpp
 
 Created by Vladimir Gusev on 01/15/2015.
 Copyright (c) 2015 scriptum.ru
 
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

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "ConcurrentQueue.h"
#include "CinderVideoStreamClient.h"
#include "cinder/app/RendererGl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

static const int WIDTH = 1280, HEIGHT = 720;

typedef CinderVideoStreamClient<uint8_t> CinderVideoStreamClientUint8;

class _TBOX_PREFIX_App : public App {
public:
    void prepareSettings( Settings *settings );
    void setup();
    void keyDown( KeyEvent event );
    void update();
    void draw();
    void shutdown();
    
private:
    
    gl::TextureRef	mTexture;
    
    void threadLoop();
    
    std::shared_ptr<std::thread> mClientThreadRef;
    
    uint8_t * mData;
    SurfaceRef mStreamSurface;
    
    std::string* mClientStatus;
    std::string mStatus;
    
    ph::ConcurrentQueue<uint8_t*>* queueFromServer;
};

void _TBOX_PREFIX_App::threadLoop()
{
    while (true) {
        try {
            std::shared_ptr<CinderVideoStreamClientUint8> s = std::shared_ptr<CinderVideoStreamClientUint8>(new CinderVideoStreamClientUint8("localhost","3333"));
            s.get()->setup(queueFromServer, mClientStatus, WIDTH*HEIGHT*3);  //3 - for RGB mode
            s.get()->run();
        }
        catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }
}

void _TBOX_PREFIX_App::prepareSettings( Settings *settings )
{
    settings->setTitle("CinderVideoStreamClient");
}

void _TBOX_PREFIX_App::setup()
{
    //setFrameRate(30);
    mClientStatus = new std::string();
    queueFromServer = new ph::ConcurrentQueue<uint8_t*>();
    mClientThreadRef = std::shared_ptr<std::thread>(new thread(boost::bind(&_TBOX_PREFIX_App::threadLoop, this)));
    mClientThreadRef->detach();
    mStreamSurface = Surface::create(WIDTH, HEIGHT, true, SurfaceChannelOrder::RGB);
    
    mStatus.assign("Starting");
}

void _TBOX_PREFIX_App::keyDown( KeyEvent event )
{
    if( event.getChar() == 'f' )
        setFullScreen( ! isFullScreen() );
}

void _TBOX_PREFIX_App::shutdown(){
    if (mData) delete mData;
    if (queueFromServer) delete queueFromServer;
}
void _TBOX_PREFIX_App::update()
{
    if (queueFromServer->try_pop(mData)){
        memcpy(mStreamSurface->getData(), mData, WIDTH * HEIGHT * 3);
        mTexture = gl::Texture::create( *mStreamSurface );
    }
    mStatus.assign("Client: ").append(std::to_string((int)getFrameRate())).append(" fps: ").append(*mClientStatus);
}

void _TBOX_PREFIX_App::draw()
{
    gl::clear( Color::black() );
    
    if( mTexture)
        gl::draw( mTexture, getWindowBounds() );
    
    gl::drawString(mStatus, vec2( 10 , 10  ) );
}


CINDER_APP( _TBOX_PREFIX_App, RendererGl )
