#include <synapse/socket/tcp/tcp_server.hpp>
#include <synapse/thread/pool/thread_pool.hpp>

#include <synapse/http/packet/request.hpp>
#include <synapse/http/packet/response.hpp>
#define  HTTP_BUFSIZE 4096

namespace http
{
    class http_server : public network::tcp_server
    {
        public:
            http_server   (uint16_t _port, std::string _path);

            using client_handler = std::function<void(network::tcp*, request)>;
            client_handler         on_http_client;

        private:
            uint16_t    http_port;
            std::string http_path;
    };

    http::http_server::http_server   (uint16_t _port, std::string _path) : network::tcp_server("0.0.0.0", _port)
    {
        on_client         = [&](network::tcp* _cl)
                            {
                                char*  _cl_msg = new char[HTTP_BUFSIZE];
                                memset(_cl_msg, 0x00, HTTP_BUFSIZE);

                                _cl              ->recv((uint8_t*)_cl_msg, HTTP_BUFSIZE);
                                if(on_http_client) on_http_client(_cl, request(_cl_msg));
                                else               delete[]  _cl_msg;
                            };
    }
}