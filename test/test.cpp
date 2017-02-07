#include <iostream>
#include <sstream>
#include <unistd.h>
#include <map>
#include <queue>
#include <boost/thread.hpp>
#include <zmq.hpp>

void hello()  
{  
    std::cout<<"Hello world, I'm a thread!"<<std::endl;  
    sleep(5);
    std::cout<<"Hello world, I'm a thread!"<<std::endl; 
}  
   

void test_thread()
{
	boost::thread thrd(&hello);  
    thrd.join();  
}

void server_cmdget(zmq::socket_t* socket, const std::string& id, const std::string& request)
{
}


//////////////////////// REQ/DEALER->ROUTER ///////////////////////////////////

void server()
{
	//  Prepare our context and sockets
    zmq::context_t context (10);
    zmq::socket_t* clients_socket_  = new zmq::socket_t(context, ZMQ_ROUTER);
    clients_socket_->bind ("tcp://*:5555");

    zmq::pollitem_t items[] = { { *clients_socket_, 0, ZMQ_POLLIN, 0 } };
    while (true)
	{
	    int rc = zmq_poll (&items[0], 1, 1000);
	    if (-1 != rc)
	    {
		    //  If we got a reply, process it
		    if (items[0].revents & ZMQ_POLLIN) 
		    {
		    	zmq::message_t idmsg;
	        	clients_socket_->recv(&idmsg);
	        	std::string id((char*)idmsg.data(), idmsg.size());

	        	
	        	// SYNC
	        	zmq::message_t empty;
	        	if (id[0] != 'a')
	        		clients_socket_->recv(&empty);

			    zmq::message_t request;
			    clients_socket_->recv (&request);

			    
			    std::string req((char*)request.data(), request.size());

			    std::cout << id << ":" << req << std::endl;

			    clients_socket_->send(idmsg, ZMQ_SNDMORE);
			    //SYNC
			    if (id[0] != 'a')
			    	clients_socket_->send(empty, ZMQ_SNDMORE);
			    clients_socket_->send(request);

			}
		}

		std::cout << "timer..." << std::endl;
	}
}

///////////////////////// REQ CLIENT //////////////////////////

zmq::context_t context (1);

void client(int i)
{
	    //  Prepare our context and socket    
    try {
	    zmq::socket_t socket (context, ZMQ_REQ);
	    std::ostringstream id;
	    id << "client" << i;
		socket.setsockopt (ZMQ_IDENTITY, id.str().data(), id.str().size());

	    std::cout << "Connecting to hello world server..." << std::endl;
	    socket.connect ("tcp://localhost:5555");

	    //  Do 10 requests, waiting each time for a response
	    for (int request_nbr = 0; request_nbr != 10; request_nbr++) {
	        zmq::message_t request (6);
	        memcpy ((void *) request.data (), "Hello", 5);
	        //std::cout << "Sending Hello " << request_nbr << "..." << std::endl;
	        socket.send (request);

	        //  Get the reply.
	        zmq::message_t reply;
	        socket.recv (&reply);


	        std::string ret((char*)reply.data(), reply.size());

	        std::cout << "Received World " << request_nbr  << ":" << ret << std::endl;
	    }
	}
	catch (std::exception& err)
	{
		std::cout << "!!!!" << err.what() << std::endl;
	}
}

void test_client(int num)
{
	boost::thread_group threads;

	for (int i = 0; i < num; i++)
	{
		threads.create_thread(boost::bind(client, i));
	}

	threads.join_all();
}

////////////////////// Load Balance Server ///////////////////

void worker(int i)
{
	zmq::context_t context(1);
    zmq::socket_t worker(context, ZMQ_REQ);

	std::ostringstream id;
	id << "worker" << i;
	worker.setsockopt (ZMQ_IDENTITY, id.str().data(), id.str().size());

	worker.connect("ipc://backend.ipc");

    //  Tell backend we're ready for work
    worker.send("READY", 5);
 
    while (1) 
    {
        //  Read and save all frames until we get an empty frame
        //  In this example there is only 1 but it could be more
       	zmq::message_t client_addr_msg;
       	worker.recv(&client_addr_msg);

       	zmq::message_t empty;
       	worker.recv(&empty);
  
        //  Get request, send reply
        zmq::message_t request;
        worker.recv(&request);

        std::cout << "Worker: " << (char*)request.data() << std::endl;

        sleep(1);
        std::string reply;
        reply = "OK";

        worker.send(client_addr_msg, ZMQ_SNDMORE);
        worker.send(empty, ZMQ_SNDMORE);
        worker.send(reply.data(), reply.size());
    }
}

void server2()
{
	   //  Prepare our context and sockets
    zmq::context_t context(1);
    zmq::socket_t frontend(context, ZMQ_ROUTER);
    zmq::socket_t backend(context, ZMQ_ROUTER);

    boost::thread_group worker_threads;

    frontend.bind("tcp://*:5555"); // frontend
    backend.bind("ipc://backend.ipc");

    for (int worker_nbr = 0; worker_nbr < 3; worker_nbr++) 
    {
    	worker_threads.create_thread(boost::bind(worker, worker_nbr));
    }

    //  Logic of LRU loop
    //  - Poll backend always, frontend only if 1+ worker ready
    //  - If worker replies, queue worker as ready and forward reply
    //    to client if necessary
    //  - If client requests, pop next worker and send request to it
    //
    //  A very simple queue structure with known max size
    std::queue<std::string> worker_queue;

    while (1) {

        //  Initialize poll set
        zmq::pollitem_t items[] = {
            //  Always poll for worker activity on backend
                { backend, 0, ZMQ_POLLIN, 0 },
                //  Poll front-end only if we have available workers
                { frontend, 0, ZMQ_POLLIN, 0 }
        };
        if (worker_queue.size())
            zmq::poll(&items[0], 2, 1000);
        else
            zmq::poll(&items[0], 1, 1000);

        //  Handle worker activity on backend
        if (items[0].revents & ZMQ_POLLIN) {

            //  Queue worker address for LRU routing
            zmq::message_t worker_addr;
            backend.recv(&worker_addr);
            worker_queue.push(std::string((char*)worker_addr.data(), worker_addr.size()));

            //  Second frame is empty
            zmq::message_t empty;
            backend.recv(&empty);
      
            //  Third frame is READY or else a client reply address
            zmq::message_t client_addr_msg;
            backend.recv(&client_addr_msg);
            std::string client_addr((char*)client_addr_msg.data(), client_addr_msg.size());

            //  If client reply, send rest back to frontend
            if (client_addr.compare("READY") != 0) 
            {
            	backend.recv(&empty);

            	zmq::message_t reply;
            	backend.recv(&reply);


            	std::string ret((char*)reply.data(), reply.size());

            	std::cout << "frontend->client:" << ret << std::endl;

            	frontend.send(client_addr_msg, ZMQ_SNDMORE);
            	// SYNC
            	if (client_addr[0] != 'a')
            		frontend.send(empty, ZMQ_SNDMORE);
            	frontend.send(reply);
            }
        }
        if (items[1].revents & ZMQ_POLLIN) {

            //  Now get next client request, route to LRU worker
            //  Client request is [address][empty][request]
            zmq::message_t client_addr_msg;
            frontend.recv(&client_addr_msg);
            std::string id((char*)client_addr_msg.data(), client_addr_msg.size());


            zmq::message_t empty;
            // SYNC
            if (id[0] != 'a')
            	frontend.recv(&empty);

            zmq::message_t request;
            frontend.recv(&request);

            
            std::string worker_addr = worker_queue.front();//worker_queue [0];
            worker_queue.pop();

            std::cout << (char*)client_addr_msg.data() << " : " 
                      << (char*)request.data()
                      << " -> " << worker_addr
                      << std::endl;

            backend.send(worker_addr.data(), worker_addr.size(), ZMQ_SNDMORE);
            backend.send(empty, ZMQ_SNDMORE);
            backend.send(client_addr_msg, ZMQ_SNDMORE);
            backend.send(empty, ZMQ_SNDMORE);
            backend.send(request);
        }
    }
}

////////////////////// DEALER CLIENT ////////////////////////

void asyncclient(int i)
{
	    //  Prepare our context and socket    
    try {
	    zmq::socket_t socket (context, ZMQ_DEALER);
	    std::ostringstream id;
	    id << "aclient" << i;
		socket.setsockopt (ZMQ_IDENTITY, id.str().data(), id.str().size());

	    std::cout << "Connecting to hello world server..." << std::endl;
	    socket.connect ("tcp://localhost:5555");

		#if 0
	    //  Do 10 requests, waiting each time for a response
	    for (int request_nbr = 0; request_nbr != 10; request_nbr++) {
	        zmq::message_t request (6);
	        memcpy ((void *) request.data (), "Hello", 5);
	        //std::cout << "Sending Hello " << request_nbr << "..." << std::endl;
	        socket.send (request);

			#if 0
	        //  Get the reply.
	        zmq::message_t reply;
	        socket.recv (&reply);

	        std::cout << "Received World " << request_nbr  << ":" << (char*)reply.data() << std::endl;
	        #endif
	    }
	    #endif

	    zmq::pollitem_t items[] = { { socket, 0, ZMQ_POLLIN, 0 } };
	    while (true)
		{
		    int rc = zmq_poll (&items[0], 1, 100);
		    if (-1 != rc)
		    {
			    //  If we got a reply, process it
			    if (items[0].revents & ZMQ_POLLIN) 
			    {
			    	zmq::message_t idmsg;
		        	//socket.recv(&idmsg);

		        	zmq::message_t empty;
		        	//clients_socket_->recv(&empty);

				    zmq::message_t request;
				    socket.recv (&request);

				    std::string id((char*)idmsg.data(), idmsg.size());
				    std::string req((char*)request.data(), request.size());

				    std::cout << id << ":" << req << std::endl;

				    //clients_socket_->send(idmsg, ZMQ_SNDMORE);
				    //clients_socket_->send(empty, ZMQ_SNDMORE);
				    //clients_socket_->send(request);
				}
			}

			zmq::message_t request (6);
	        memcpy ((void *) request.data (), "Hello", 5);
	        //std::cout << "Sending Hello " << request_nbr << "..." << std::endl;
	        socket.send (request);
		}
	}
	catch (std::exception& err)
	{
		std::cout << "!!!!" << err.what() << std::endl;
	}
}

void test_async_client(int num)
{
	boost::thread_group threads;

	for (int i = 0; i < num; i++)
	{
		threads.create_thread(boost::bind(asyncclient, i));
	}

	threads.join_all();
}

//////////////////////////////// MAIN ///////////////////////

int main(int argc, const char* argv[])
{
	if (argc > 1)
	{
		int num = 1;
		if (argc > 2)
			num = atol(argv[2]);

		std::string arg1 = argv[1];
		if ("client" == arg1)
		{
			test_client(num);
		}
		else if ("aclient" == arg1)
		{
			test_async_client(num);
		}
		else if ("server" == arg1)
		{
			server();	
		}
		else if ("server2" == arg1)
		{
			server2();
		}
		else if ("thread" == arg1)
		{
			test_thread();
		}
	}


}