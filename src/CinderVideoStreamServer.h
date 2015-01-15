/*
 CinderVideoStreamServer.h
 
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

#ifndef CinderVideoStreamServer_CinderVideoStreamServer_h
#define CinderVideoStreamServer_CinderVideoStreamServer_h
#include <boost/asio.hpp>
#include <boost/bind.hpp>


using namespace boost::asio;
template <class T>
class CinderVideoStreamServer{
    public:

    CinderVideoStreamServer(unsigned short port, ph::ConcurrentQueue<uint8_t*>* queueToServer, unsigned int width, unsigned int height)
                                :mSocket(mIOService),mAcceptor(mIOService,ip::tcp::endpoint(ip::tcp::v4(), port)),mQueue(queueToServer), dSize(width*height*3*sizeof(T)){
                                    boost::asio::socket_base::reuse_address option(true);
                                    mAcceptor.set_option(option);
                                }
    void run(){

        T* data;
        boost::system::error_code ignored_error;

        while(true){
            if (mQueue->try_pop(data)){
                const mutable_buffer image_buffer(data, dSize);
                mAcceptor.accept(mSocket);
                boost::asio::write(mSocket, buffer(image_buffer), transfer_all(), ignored_error);
                mSocket.close();
            }
        }
    }
private:
    io_service mIOService;
    ip::tcp::socket mSocket;
    ip::tcp::acceptor mAcceptor;
    ph::ConcurrentQueue<T*>* mQueue;
    std::size_t dSize;

};

#endif
