#ifndef _CONTROLLERSOCKET_H_
#define _CONTROLLERSOCKET_H_

#include "ModelSocket.h"
#include "IController.h"
#include "IObserver.h"



class ControllerSocket : IController, IObservable
{
private:
    ModelSocket* mSocket;
    std::vector<ModelSocket*> vConnection;
    std::vector<IObserver*> observers;
    ModelString mString;
    pthread_t thCheckConnect;
public:
    ControllerSocket(int typeSocket)
    {
        this->name="ControllerSocket";
        mSocket = new ModelSocket();
        mSocket->type=typeSocket;
    }
    ~ControllerSocket()
    {
        if(mSocket->type == NET_CLIENT && mSocket->state == 0)
        {
            mSocket->sendStream<<"disconnect";
            while(!(mSocket->sendStream.str().empty())){}
            //int res = shutdown(mSocket->socket,SD_SEND);
        }
        if(mSocket->type == NET_SERVER)
        {
            for (int i = 0; i < vConnection.size(); i++)
            {
                vConnection[i]->sendStream<<"disconnect";
                while(!(vConnection[i]->sendStream.str().empty())){}
                CloseSocket(vConnection[i]);
            }
        }

        #if TARGET_PLATFORM == PLATFORM_WINDOWS
        WSACleanup();
        Sleep(1000);
        #endif
        #if TARGET_PLATFORM == PLATFORM_UNIX
        usleep(1000);
        #endif
        CloseSocket(mSocket);

        printf(" ~ControllerSocket: \n");
        exit(0);
    }
    void AddObserver(IObserver* o)
	{
        if(DEBUG==1){printf("ControllerSocket add: %s\n",((IController*)o)->name.c_str());}
		observers.push_back(o);
	}
    void RemoveObserver(IObserver* o)
	{
		std::vector<IObserver*>::iterator itErase = observers.begin();
		for (itErase; itErase != observers.end(); itErase++)
		{
			if (*itErase == o) { observers.erase(itErase); break; }
		}
	}
    void Notify(ModelString mString)
	{
		for (auto o : this->observers)
		{
			((IController*)o)->UpdateController(mString);
		}
	}
    void UpdateController(ModelString mString) override
	{
        mSocket->str=mString.str;
        this->Input();
	}   
    int InitSocket(std::string ipAddr,int port)
    {
        mSocket->recvStream.str("");
        mSocket->sendStream.str("");
        mSocket->ipaddr = ipAddr;
        mSocket->port = port;
        #if TARGET_PLATFORM == PLATFORM_WINDOWS
		mSocket->wsOK = WSAStartup(MAKEWORD(2, 2), &mSocket->wsdata);
		if (mSocket->wsOK != 0) { std::cout << "Can't start winsock" << std::endl; }
        #endif
        
		//SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		mSocket->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		mSocket->sockaddr.sin_family = AF_INET; //AF_INET6
		mSocket->sockaddr.sin_port = htons(port);
        #if TARGET_PLATFORM == PLATFORM_WINDOWS
		mSocket->sockaddr.sin_addr.S_un.S_addr = inet_addr(ipAddr.c_str());// ADDR_ANY;// INADDR_ANY;//
        #endif
        #if TARGET_PLATFORM == PLATFORM_UNIX
        mSocket->sockaddr.sin_addr.s_addr = inet_addr(ipAddr.c_str());
        #endif
		mSocket->sockaddr_size = sizeof(mSocket->sockaddr);

        if(mSocket->type==NET_SERVER)
        {
            pthread_create(&(this->thCheckConnect),NULL,ControllerSocket::ThreadStubChechConnection,(void*)this);
            //pthread_join(this->th;, NULL);
            pthread_detach(this->thCheckConnect);
        } 

        return 0;
    }
    int WaitConnect()
    {
        if(mSocket->type==NET_SERVER)
        {
            //биндит свой, слушает свой
            #if TARGET_PLATFORM == PLATFORM_WINDOWS
            int rbind = bind(mSocket->socket,(const SOCKADDR*)&(mSocket->sockaddr),sizeof(mSocket->sockaddr));
            #endif
            #if TARGET_PLATFORM == PLATFORM_UNIX
            int rbind = bind(mSocket->socket,(const sockaddr*)&(mSocket->sockaddr),sizeof(mSocket->sockaddr));
            #endif
            if(rbind<0){printf(" Error bind, socket used: %d",rbind);this->~ControllerSocket();}
            else if(rbind==0){printf(" Success bind: %d",rbind);} 
            else if(rbind>0){printf(" rbind>0: %d",rbind);}
            
            int rlisten = listen(mSocket->socket,1);
            if(rlisten<0){printf(" rlisten<0: %d\n",rlisten);}
            else if(rlisten==0){printf(" Success listen: %d\n",rlisten);} 
            else if(rlisten>0){printf(" rlisten>0: %d\n",rlisten);}
            
            #if TARGET_PLATFORM == PLATFORM_WINDOWS
            SOCKET connect_socket;
            SOCKADDR_IN connect_addr;
            int connect_addr_size = sizeof(connect_addr);
            #endif
            #if TARGET_PLATFORM == PLATFORM_UNIX
            int connect_socket;
            struct sockaddr_in connect_addr;
            socklen_t connect_addr_size = sizeof(connect_addr);
            #endif

            //ожидание входящего подключения
            #if TARGET_PLATFORM == PLATFORM_WINDOWS
            while(connect_socket = accept(mSocket->socket,(SOCKADDR*)&connect_addr,&connect_addr_size))
            #endif
            #if TARGET_PLATFORM == PLATFORM_UNIX
            while(connect_socket = accept(mSocket->socket,(sockaddr*)&connect_addr,&connect_addr_size))
            #endif
            {
                ModelSocket *connectSocket = new ModelSocket();
                connectSocket->socket=connect_socket;
                connectSocket->sockaddr=connect_addr;
                connectSocket->sockaddr_size=connect_addr_size;

                #if TARGET_PLATFORM == PLATFORM_WINDOWS
                connectSocket->wsOK = WSAStartup(MAKEWORD(2, 2), &connectSocket->wsdata);
                if (connectSocket->wsOK != 0) { std::cout << "Can't start winsock" << std::endl; }
                #endif

                #if TARGET_PLATFORM == PLATFORM_WINDOWS
                DWORD nonBlocking = 1;
                if(ioctlsocket(connectSocket->socket,FIONBIO,&nonBlocking)!=0)
                {
                    printf("failed to set non-blocking socket\n");
                    Sleep(2000);
                    exit(0);
                }
                #endif
                
                vConnection.push_back(connectSocket);

				printf("%s", inet_ntoa(vConnection[vConnection.size()-1]->sockaddr.sin_addr));
				printf(": %d", ntohs(vConnection[vConnection.size()-1]->sockaddr.sin_port));
                printf(": connected\n");

                pthread_create(&(vConnection[vConnection.size()-1]->th),NULL,ControllerSocket::ThreadStub,(void*)this);
                //pthread_join(th, NULL);
		        pthread_detach(vConnection[vConnection.size()-1]->th);
            }
        }
        if(mSocket->type==NET_CLIENT)
        {
            //подключается к серверу
            #if TARGET_PLATFORM == PLATFORM_WINDOWS
            int con = connect(mSocket->socket,(const SOCKADDR*)&(mSocket->sockaddr),sizeof(mSocket->sockaddr));
            if(con!=0){printf("Server disconnect\n");Sleep(1000);exit(0);}
            #endif
            #if TARGET_PLATFORM == PLATFORM_UNIX
            int con = connect(mSocket->socket,
                    reinterpret_cast<const sockaddr*>(&mSocket->sockaddr),
                    sizeof(mSocket->sockaddr));
            if(con!=0){printf("Server disconnect\n");usleep(1000);exit(0);}
            #endif

            #if TARGET_PLATFORM == PLATFORM_WINDOWS
            DWORD nonBlocking = 1;
            if(ioctlsocket(mSocket->socket,FIONBIO,&nonBlocking)!=0)
            {
                printf("failed to set non-blocking socket\n");
                Sleep(2000);
                exit(0);
            }
            #endif
            #if TARGET_PLATFORM == PLATFORM_UNIX
            int nonBlocking = 1;
            if(fcntl(mSocket->socket,F_SETFL,O_NONBLOCK,nonBlocking)==-1)
            {
                printf("WaitConnect: failed to set non-blocking socket\n"); 
                usleep(2000);
                exit(0);
            }
            #endif

            //value_client_port = mSocket->sockaddr.sin_port;
            printf("%s", inet_ntoa(mSocket->sockaddr.sin_addr));
			printf(": %d", ntohs(mSocket->sockaddr.sin_port));
            printf(": connected \n");

            pthread_create(&(mSocket->th),NULL,ControllerSocket::ThreadStub,(void*)this);
            //pthread_join(th, NULL);
		    pthread_detach(mSocket->th);
        }
        return 0;
    }
    ///////////////////////////////////////////////
    virtual void ThreadProcWait()
   	{
        if(mSocket->type == NET_SERVER)
        {
            //ожадиет клиентов
		    this->WaitConnect();
        }
        if(mSocket->type == NET_CLIENT)
        {
            //работает с заданным сокетом
            //this->WaitConnect(mSocket);
        }
   	}
    static void *ThreadStubWait(void* self)
    {
		((ControllerSocket*)self)->ThreadProcWait();
		return 0;
    }
    ////////////////////////////////////////////
    void ChechConnection()
    {
        while(true)
        {
            #if TARGET_PLATFORM == PLATFORM_WINDOWS
            Sleep(500);
            #endif
            #if TARGET_PLATFORM == PLATFORM_UNIX
            usleep(10);
            #endif

            for (int i = 0; i < vConnection.size(); i++)
            {
                int sb = send(vConnection[i]->socket,"",strlen(""),0);
                if(sb<0)
                {
                    this->ClearListClient(vConnection[i]);
                }
            }
        }
    }
    virtual void ThreadProcChechConnection()
   	{
        this->ChechConnection();
   	}
    static void *ThreadStubChechConnection(void* self)
    {
		((ControllerSocket*)self)->ThreadProcChechConnection();
		return 0;
    }
    ///////////////////////////////////////////////
    virtual void ThreadProc()
   	{
        if(mSocket->type==NET_SERVER)
        {
            //работает с подключившимся сокетом
		    this->ExchangeData(vConnection[vConnection.size()-1]);
        }
        if(mSocket->type==NET_CLIENT)
        {
            //работает с заданным сокетом
            this->ExchangeData(mSocket);
        }
   	}
    static void *ThreadStub(void* self)
    {
		((ControllerSocket*)self)->ThreadProc();
		return 0;
    }
    ///////////////////////////////////////////////
    void ExchangeData(ModelSocket *connectSocket)
    {
        int rb;
        int sb;
        
        //WSAEventSelect(connectSocket->socket,0,FD_READ);
        while(connectSocket->state!=2)
        {
            #if TARGET_PLATFORM == PLATFORM_UNIX
            int nonBlocking = 1;
            if(fcntl(connectSocket->socket,F_SETFL,O_NONBLOCK,nonBlocking) == -1)
            {
                printf("ExchangeData: failed to set non-blocking socket\n"); 
                usleep(2000);
                //exit(0);
            }
            #endif

            //sleep для разругрузки процессора
            #if TARGET_PLATFORM == PLATFORM_WINDOWS
            Sleep(10);
            #endif
            #if TARGET_PLATFORM == PLATFORM_UNIX
            usleep(10);
            #endif

            while(!(connectSocket->sendStream.str().empty()))
            {
                //sb = send(connectSocket->socket,connectSocket->sendStream.str().c_str(),connectSocket->sendStream.str().length(),0);

                if(mSocket->type == NET_CLIENT)
                {
                    //error send
                    if(sb<0)
                    {
                        mSocket->sstate="";
                        mSocket->state=1;this->~ControllerSocket();
                    }
                }
                if(mSocket->type == NET_SERVER)
                {
                    if(sb<0){this->ClearListClient(connectSocket);} 
                }
                
                if(sb>0){
                connectSocket->sendStream.str("");}
            }

            rb = recv(connectSocket->socket,(char*)(connectSocket->cstr),sizeof(connectSocket->cstr)-1,0);

            
            //if(rb<0){printf(" rb<0: %d",rb);} //success connect
            //if(rb==0){printf(" rb==0: %d",rb);} //disconnect
            if(mSocket->type == NET_CLIENT)
            {
                if(rb==0){this->~ControllerSocket();}
            }
            if(mSocket->type == NET_SERVER)
            {
                if(rb==0){CloseSocket(connectSocket);}
            }

            static std::string state_print = "print";
            
            #if TARGET_PLATFORM == PLATFORM_WINDOWS
            if(rb>=0)
            #endif
            #if TARGET_PLATFORM == PLATFORM_UNIX
            if(rb>0) 
            #endif
            {
                connectSocket->cstr[rb]=0; //clear after rb

                if(RegexHttp(connectSocket->cstr,"HTTP") == 1)
                {
                    state_print = "noprint";
                    ResponseHttp(connectSocket);
                }

                if(strcmp(connectSocket->cstr,"disconnect") == 0)
                {
                    if(mSocket->type == NET_CLIENT)
                    {
                        //mSocket->state=1;
                        mSocket->state=1;
                        this->~ControllerSocket();
                    }
                    if(mSocket->type == NET_SERVER)
                    {
                        CloseSocket(connectSocket);
                        //ClearListClient(connectSocket);
                    }
                }

                if(state_print == "print")
                {
                    printf("%s", inet_ntoa(connectSocket->sockaddr.sin_addr));
                    printf(": %d", ntohs(connectSocket->sockaddr.sin_port));
                    printf(": %s\n",connectSocket->cstr);

                    connectSocket->recvString=connectSocket->cstr;

                    mString.str=connectSocket->recvString;
                    Notify(mString);

                    strcpy(connectSocket->cstr, "");
                    connectSocket->recvStream.clear();
                    connectSocket->recvString.clear();
                }
            }
        }
    }
    int Input()
    {
        if(mSocket->type == NET_SERVER)
        {
            if(mSocket->str=="exit/")
            {
                this->~ControllerSocket();
            }
            else
            {
                for (int i = 0; i < vConnection.size(); i++)
                {
                    vConnection[i]->sendStream<<mSocket->str;
                }
                mSocket->str = "";
            }
        }
        else if(mSocket->type == NET_CLIENT)
        {
            if(mSocket->str=="exit/")
            {
                this->~ControllerSocket();
            }
            else
            {
                mSocket->sendStream<<mSocket->str;
                mSocket->str = "";
            }
        }
        return mSocket->state;
    }
    void UpdateSocket()
    {
        if(mSocket->type == NET_SERVER)
        {
            pthread_create(&(mSocket->th),NULL,ControllerSocket::ThreadStubWait,(void*)this);
            //pthread_join(th, NULL);
		    pthread_detach(mSocket->th);
        }
        if(mSocket->type == NET_CLIENT)
        {
            WaitConnect();
        }
    }
    void ClearListClient(ModelSocket *mSocket)
    {
        for (int i = 0; i < vConnection.size(); i++)
        {
            if(vConnection[i]->sockaddr.sin_port == mSocket->sockaddr.sin_port)
            {
                vConnection[i]->state=2;
                vConnection[i]->sstate="";
                vConnection[i]->sendStream.str("");

                CloseSocket(vConnection[i]);

                //int cancle = pthread_cancel(vConnection[vConnection.size()-1]->th);
                //if (cancle != 0){printf("Error cancle thread \n");break;}
                //else{printf("Success cancle thread \n");}

                printf("%s", inet_ntoa(vConnection[i]->sockaddr.sin_addr));
				printf(": %d", ntohs(vConnection[i]->sockaddr.sin_port));
                printf(": disconnect\n");

                vConnection.erase(vConnection.begin()+i);
                printf("Count connection: %ld\n",vConnection.size());
                break;
            }
        }
    }
    int CloseSocket(ModelSocket *modelSocket)
    {
        #if TARGET_PLATFORM == PLATFORM_WINDOWS
        closesocket(modelSocket->socket);
        #endif
        #if TARGET_PLATFORM == PLATFORM_UNIX
        close(modelSocket->socket);
        #endif
        return 0;
    }
    //////////////////////////httpvoid 
    int RegexHttp(std::string msg,std::string word)
    {
        std::string str = msg;
        std::regex re{"("+word+")"};
        std::smatch match;

        for (std::sregex_iterator ibegin{str.begin(),str.end(),re},iend;ibegin!=iend;++ibegin)
        {
            if(std::regex_search(str, match, re)) {
                return 1;
            }
        }
        return 0;
    }
    void ResponseHttp(ModelSocket *connectSocket)
    {
        std::stringstream response; 
        std::stringstream response_body; 

        response_body << "<link rel=icon href=data:;base64,=>"
            << "<title>spectrtaldev.ru</title>"
            << "<style>body{background-color:#000;}</style>"
            << "<h1 style=color:green;text-align:center;margin-top:5em;>spectraldev.ru</h1>";

        response << "HTTP/1.1 200 OK\r\n"
            << "Version: HTTP/1.1\r\n"
            << "Content-Type: text/html; charset=utf-8\r\n"
            << "Content-Length: " << response_body.str().length()
            << "\r\n\r\n"
            << response_body.str();

        int result = send(connectSocket->socket, response.str().c_str(),
            response.str().length(), 0);
    }
};

#endif