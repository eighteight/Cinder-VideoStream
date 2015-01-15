/*
  CinderVideoStreamClient.h
  CinderVideoStreamClient

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

#ifndef CaptureTCPServer_TCPServer_h
#define CaptureTCPServer_TCPServer_h
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

using namespace boost::asio::ip;

template <class T>
class CinderVideoStreamClient{
    public:

    CinderVideoStreamClient(std::string host, std::string service):mIOService(), mHost(host), mService(service)
        {
        }
    ~CinderVideoStreamClient(){
        if (mData) delete [] mData;
    }
    void setup(ph::ConcurrentQueue<T*>* queueToServer, std::string* status, std::size_t dataSize){
        mQueue = queueToServer;
        mStatus = status;
        mDataSize = dataSize;
        mData = new T[mDataSize];
    }
    void run(){
        tcp::resolver resolver(mIOService);
        boost::array<T, 65536> temp_buffer;
        size_t len, iSize;
        tcp::resolver::query query(tcp::v4(), mHost, mService);
        while(true){
            try
            {
                tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
                tcp::resolver::iterator end;
                
                tcp::socket socket(mIOService);
                boost::system::error_code error = boost::asio::error::host_not_found;
                
                while (error && endpoint_iterator != end)
                {
                    socket.close();
                    socket.connect(*endpoint_iterator++, error);
                }
                if (error)
                    throw boost::system::system_error(error);

                iSize = 0;
                for (;;)
                {
                    len = socket.read_some(boost::asio::buffer(temp_buffer), error)/sizeof(T);

                    //memcpy(mData, &temp_buffer[0], temp_buffer.size());
                    //copy( temp_buffer.begin(), temp_buffer.begin(), mData);
                    for(int i = 0; i < len; i++) {
                        mData[i+iSize] = temp_buffer[i];
                    }

                    iSize += len;
                    
                    if (error == boost::asio::error::eof)
                        break; // Connection closed cleanly by peer.
                    else if (error)
                        throw boost::system::system_error(error); // Some other error.
                }
                mQueue->push(mData);
                (*mStatus).assign("Capturing");
            }
            catch (std::exception& e)
            {
                (*mStatus).assign(e.what(), strlen(e.what()));
            }
        }
    }
private:
    boost::asio::io_service mIOService;
    ph::ConcurrentQueue<T*>* mQueue;
    std::string mService;
    std::string mHost;
    std::string* mStatus;
    std::size_t mDataSize;
    T* mData;
};

#endif
