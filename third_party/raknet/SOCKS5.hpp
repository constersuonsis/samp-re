#pragma once

/* modified by AdCKuY_DpO4uLa */
/* https://www.blast.hk/members/229228/ */
/* https://github.com/DpO4uLa */
/* this class support only SOCKS5 UDP proxy (no auth + auth) */

/* SOCKS5 RFC */
/* https://www.rfc-es.org/rfc/rfc1928-es.txt */
/* https://www.rfc-es.org/rfc/rfc1929-es.txt */

#ifndef __SOCKS5_HPP
#define __SOCKS5_HPP

#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
#include <functional>
#include <chrono>
#if defined(_WIN32)
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "WS2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h> 
#include <cstring> // std::memcpy
#endif

// unified methods
namespace
{
	int GetLastSocketError()
	{
#if defined(_WIN32)
		return WSAGetLastError();
#else
		return errno;
#endif
	};

#if defined(_WIN32)
	#define EWOULDBLOCK WSAEWOULDBLOCK
#else
	#define EWOULDBLOCK EAGAIN
	#define closesocket close
	#define INVALID_SOCKET -1
	#ifndef SOCKET_ERROR
	#define SOCKET_ERROR INVALID_SOCKET
	#endif
	typedef int SOCKET;
#endif
}

//define this if you want to log the status of the connection to the proxy
//#define SOCKS5_LOG

namespace SOCKS5
{
#pragma pack(push, 1)
	struct AuthRequestHeader
	{
		std::uint8_t		byteVersion;
		std::uint8_t		byteAuthMethodsCount;
		std::uint8_t		byteMethods[1];
	};
	struct AuthRespondHeader
	{
		std::uint8_t	byteVersion;
		std::uint8_t		byteAuthMethod;
	};
	struct AuthRespondHeaderAuth //only for proxy with auth
	{
		std::uint8_t		byteVersion;
		std::uint8_t		byteStatus;
	};
	struct ConnectRequestHeader
	{
		std::uint8_t		byteVersion;
		std::uint8_t		byteCommand;
		std::uint8_t		byteReserved;
		std::uint8_t		byteAddressType;
		std::uint32_t		ulAddressIPv4;
		std::uint16_t		usPort;
	};
	struct ConnectRespondHeader
	{
		std::uint8_t		byteVersion;
		std::uint8_t		byteResult;
		std::uint8_t		byteReserved;
		std::uint8_t		byteAddressType;
		std::uint32_t		ulAddressIPv4;
		std::uint16_t		usPort;
	};
	struct UDPDatagramHeader
	{
		std::uint16_t		usReserved;
		std::uint8_t		byteFragment;
		std::uint8_t		byteAddressType;
		std::uint32_t		ulAddressIPv4;
		std::uint16_t		usPort;
	};
#pragma pack( pop )

	//error codes
	enum class eSocks5Error
	{
		eNone,
		eFailedToCreateSocket,
		eFailedToEnableNonBlockingMode,
		eFailedToConnectToTCPServer,
		eSuccessfulConnectionToRemoteTCPServer,
		eRemoteTCPServerClosedTheConnection,
		eNonBlockingSocketOperationCouldNotBeCompletedImmediately,
		eSelectFailed,
		eTimeoutConnectingToRemoteTCPServer,
		eProcessAuthentication,
		eAuthenticationError,
		eAuthenticationSuccessful,
		eProcessAuthorization,
		eAuthorizationError,
		eAuthorizationSuccessful,
		eQueryingDataAboutARemoteUDPServer,
		eFailedToQueryingDataAboutARemoteUDPServer,
		eRemoteUDPServerDataSuccessfullyReceived,
		eNetworkAddressOrPortCouldNotBeInitialized,
		eProxyInitializedSuccessfully
	};
	class SOCKS5 {
	private:
		SOCKET m_sockTCP; //socket of proxy
		sockaddr_in m_proxyServerAddr; //public addr and port of proxy
		std::uint32_t m_proxyIP; //network addr of proxy
		std::uint16_t m_proxyPort; //network port of proxy
		std::string m_thisIP; //connected proxy ip
		std::string m_thisPORT; //connected proxy port
		std::string m_thisLogin; //connected proxy login (only auth)
		std::string m_thisPassword; //connected proxy password (only auth)
		bool m_bIsStarted; //is proxy connected
		bool m_bIsValidReceiving; //set true if you successfully connected to server
		bool m_bIsReceivingByProxy; //set true if you want to redirect traffic to proxy
		std::function<void(SOCKS5*, eSocks5Error)> m_Handler; //error handler
		std::chrono::milliseconds m_LastConnectionCheck; 
		enum class eProxyStatus //proxy status on asynchronous connection
		{
			eUnknown,
			eFailed,
			eProcessConnectToSocket,
			eConnectedToSocket,
			eSendAuthRequestHeader,
			eSendConnectRequestHeader,
			eSendAuthRequestPasswd,
			eAuthRespondHeaderAuth,
			eConnectRespondHeader,
			eInitialized
		} m_ProxyStatus;
		bool m_bIsTimerEnabled;
		bool m_bIsProcessing;
		auto Send(void* buffer, std::size_t size) -> bool
		{
			// Checking if a socket is ready for writing via select
			fd_set writefds{};
			FD_ZERO(&writefds);
			FD_SET(this->m_sockTCP, &writefds);

			timeval select_timeout{};
			select_timeout.tv_sec = 0;
			select_timeout.tv_usec = 0;

			int select_result = select(static_cast<int>(this->m_sockTCP) + 1, nullptr, &writefds, nullptr, &select_timeout);

			if (select_result > 0 && FD_ISSET(this->m_sockTCP, &writefds))
			{
				// The socket is ready to write.
				int result = send(this->m_sockTCP, static_cast<const char*>(buffer), static_cast<int>(size), 0);

				if (result > 0)
				{
					this->m_bIsTimerEnabled = false;
					return true;
				}
				else if (result == 0)
				{
					// Connection closed
#if defined (SOCKS5_LOG)
					printf("[CProxy::Send->Error]: Connection closed by peer\n");
#endif
					closesocket(this->m_sockTCP);
					if (this->m_Handler)
					{
						this->m_Handler(this, eSocks5Error::eRemoteTCPServerClosedTheConnection);
					}
					this->m_ProxyStatus = eProxyStatus::eFailed;
					this->m_bIsTimerEnabled = false;
					return false;
				}
				else
				{
					// Error sending
					int error = GetLastSocketError();
					if (error != EWOULDBLOCK)
					{
#if defined (SOCKS5_LOG)
						printf("[CProxy::Send->Error]: Send error: %d\n", error);
#endif
						closesocket(this->m_sockTCP);
						if (this->m_Handler)
						{
							this->m_Handler(this, eSocks5Error::eNonBlockingSocketOperationCouldNotBeCompletedImmediately);
						}
						this->m_ProxyStatus = eProxyStatus::eFailed;
						this->m_bIsTimerEnabled = false;
						return false;
					}
					// WSAEWOULDBLOCK - socket is not ready again, keep trying
					return false;
				}
			}
			else if (select_result == 0)
			{
				// select() timeout, socket not ready for writing
				return false;
			}
			else
			{
				// Error select()
				int error = GetLastSocketError();
#if defined(SOCKS5_LOG)
				printf("[CProxy::Send->Error]: Select error: %d\n", error);
#endif
				closesocket(this->m_sockTCP);
				if (this->m_Handler)
				{
					this->m_Handler(this, eSocks5Error::eSelectFailed);
				}
				this->m_ProxyStatus = eProxyStatus::eFailed;
				this->m_bIsTimerEnabled = false;
				return false;
			}
		}
		auto Receive(void* buffer, std::size_t size) -> bool
		{
			while (true) {
				// Checking if there is data to read
				fd_set readfds{};
				FD_ZERO(&readfds);
				FD_SET(this->m_sockTCP, &readfds);

				timeval select_timeout{};
				select_timeout.tv_sec = 0;
				select_timeout.tv_usec = 0;

				int select_result = select(static_cast<int>(this->m_sockTCP) + 1, &readfds, nullptr, nullptr, &select_timeout);

				if (select_result > 0 && FD_ISSET(this->m_sockTCP, &readfds)) 
				{
					// Data is readable
					int bytesReceived = recv(this->m_sockTCP, static_cast<char*>(buffer), static_cast<int>(size), 0);

					if (bytesReceived > 0) {
						return true;
					}
					else if (bytesReceived == 0) {
						// The connection was closed by the remote peer.
#if defined(SOCKS5_LOG)
						printf("[CProxy::Receive->Error]: Connection closed by peer\n");
#endif
						closesocket(this->m_sockTCP);
						if (this->m_Handler) {
							this->m_Handler(this, eSocks5Error::eRemoteTCPServerClosedTheConnection);
						}
						this->m_ProxyStatus = eProxyStatus::eFailed;
						this->m_bIsTimerEnabled = false;
						return false;
					}
					else 
					{
						// Error while reading
						int error = GetLastSocketError();
						if (error != EWOULDBLOCK) {
#if defined(SOCKS5_LOG)
							printf("[CProxy::Receive->Error]: Receive error: %d\n", error);
#endif
							closesocket(this->m_sockTCP);
							if (this->m_Handler) {
								this->m_Handler(this, eSocks5Error::eNonBlockingSocketOperationCouldNotBeCompletedImmediately);
							}
							this->m_ProxyStatus = eProxyStatus::eFailed;
							this->m_bIsTimerEnabled = false;
							return false;
						}
						// If WSAEWOULDBLOCK - continue waiting
						return false; // check this later
					}
				}
				else if (select_result == 0) 
				{
					// select() timeout, continue waiting
					return false;
				}
				else 
				{
					// Error select()
					int error = GetLastSocketError();
#if defined(SOCKS5_LOG)
					printf("[CProxy::Receive->Error]: Select error: %d\n", error);
#endif
					closesocket(this->m_sockTCP);
					if (this->m_Handler) {
						this->m_Handler(this, eSocks5Error::eSelectFailed);
					}
					this->m_ProxyStatus = eProxyStatus::eFailed;
					this->m_bIsTimerEnabled = false;
					return false;
				}
			}
		}
	public:
		SOCKS5(const std::string ProxyIP, const std::string ProxyPort, const std::string ProxyLogin = "", const std::string ProxyPassword = "") :
			m_sockTCP(INVALID_SOCKET),
			m_proxyServerAddr{},
			m_proxyIP{ inet_addr(ProxyIP.c_str()) },
			m_proxyPort{ htons(std::atoi(ProxyPort.c_str())) },
			m_thisIP{ ProxyIP },
			m_thisPORT{ ProxyPort },
			m_thisLogin{ ProxyLogin },
			m_thisPassword{ ProxyPassword },
			m_bIsStarted(false),
			m_bIsValidReceiving(false),
			m_bIsReceivingByProxy(false),
			m_ProxyStatus{ eProxyStatus::eUnknown },
			m_Handler{},
			m_LastConnectionCheck{},
			m_bIsTimerEnabled{ false },
			m_bIsProcessing{ false }
		{};
		SOCKS5() :
			m_sockTCP(INVALID_SOCKET),
			m_proxyServerAddr{},
			m_proxyIP{},
			m_proxyPort{},
			m_thisIP{},
			m_thisPORT{},
			m_thisLogin{},
			m_thisPassword{},
			m_bIsStarted(false),
			m_bIsValidReceiving(false),
			m_bIsReceivingByProxy(false),
			m_ProxyStatus{ eProxyStatus::eUnknown },
			m_Handler{},
			m_LastConnectionCheck{},
			m_bIsTimerEnabled{ false },
			m_bIsProcessing{ false }
		{};
		~SOCKS5() {
			if (m_bIsStarted)
				Shutdown();
		};
		SOCKS5& operator= (const SOCKS5& prox) 
		{
			this->m_sockTCP = prox.m_sockTCP;
			this->m_proxyServerAddr = prox.m_proxyServerAddr;
			this->m_proxyIP = prox.m_proxyIP;
			this->m_proxyPort = prox.m_proxyPort;
			this->m_thisIP = prox.m_thisIP;
			this->m_thisPORT = prox.m_thisPORT;
			this->m_thisLogin = prox.m_thisLogin;
			this->m_thisPassword = prox.m_thisPassword;
			this->m_bIsStarted = prox.m_bIsStarted;
			this->m_bIsValidReceiving = prox.m_bIsValidReceiving;
			this->m_bIsReceivingByProxy = prox.m_bIsReceivingByProxy;
			this->m_ProxyStatus = prox.m_ProxyStatus;
			this->m_Handler = prox.m_Handler;
			this->m_LastConnectionCheck = prox.m_LastConnectionCheck;
			this->m_bIsTimerEnabled = prox.m_bIsTimerEnabled;
			this->m_bIsProcessing = prox.m_bIsProcessing;
			return *this;
		};
		//register error handler
		auto RegisterHandler(std::function<void(SOCKS5*, eSocks5Error)> handler) -> void
		{
			this->m_Handler = handler;
		}
		//update proxy network
		auto Update(void) -> void
		{
			switch (this->m_ProxyStatus)
			{
			case eProxyStatus::eUnknown:
			case eProxyStatus::eFailed:
				return;
			case eProxyStatus::eProcessConnectToSocket:
			{
				// timeout check
				auto now = std::chrono::steady_clock::now();
				if (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) -
					this->m_LastConnectionCheck > std::chrono::milliseconds(15000))
				{
					closesocket(this->m_sockTCP);
#if defined(SOCKS5_LOG)
					printf("[CProxy::Start->Error]: Connection timeout. (Socket error: %d)\n", GetLastError());
#endif
					if (this->m_Handler)
					{
						this->m_Handler(this, eSocks5Error::eTimeoutConnectingToRemoteTCPServer);
					}
					this->m_bIsProcessing = false;
					this->m_ProxyStatus = eProxyStatus::eFailed;
					return;
				}

				fd_set writefds{};
				FD_ZERO(&writefds);
				FD_SET(this->m_sockTCP, &writefds);

				timeval timeout{};
				timeout.tv_sec = 0;
				timeout.tv_usec = 0;

				int result = select(static_cast<int>(this->m_sockTCP) + 1, nullptr, &writefds, nullptr, &timeout);
				if (result > 0 && FD_ISSET(this->m_sockTCP, &writefds))
				{
					int error = 0;
					socklen_t len = sizeof(error);
					if (getsockopt(this->m_sockTCP, SOL_SOCKET, SO_ERROR, (char*)&error, &len) == 0 && error == 0)
					{
						if (this->m_Handler)
						{
							this->m_Handler(this, eSocks5Error::eSuccessfulConnectionToRemoteTCPServer);
						}
						this->m_ProxyStatus = eProxyStatus::eConnectedToSocket;
						return;
					}
					else
					{
						closesocket(this->m_sockTCP);
#if defined(SOCKS5_LOG)
						printf("[CProxy::Start->Error]: Connection failed. (Socket error: %d)\n", error);
#endif
						if (this->m_Handler)
						{
							this->m_Handler(this, eSocks5Error::eFailedToConnectToTCPServer);
						}
						this->m_bIsProcessing = false;
						this->m_ProxyStatus = eProxyStatus::eFailed;
						return;
					}
				}
				else if (result == 0)
				{
					// select() timeout, just wait for next iteration
				}
				else if (result == INVALID_SOCKET)
				{
					int error = GetLastSocketError();
					closesocket(this->m_sockTCP);
#if defined(SOCKS5_LOG)
					printf("[CProxy::Start->Error]: Select failed. (Socket error: %d)\n", error);
#endif
					if (this->m_Handler)
					{
						this->m_Handler(this, eSocks5Error::eSelectFailed);
					}
					this->m_bIsProcessing = false;
					this->m_ProxyStatus = eProxyStatus::eFailed;
					return;
				}
				break;
			}
			case eProxyStatus::eConnectedToSocket:
			{
			/*
				+----+----------+----------+
				|VER | NMETHODS | METHODS  |
				+----+----------+----------+
				| 1  |    1     | 1 to 255 |
				+----+----------+----------+
			*/
				AuthRequestHeader ahead{};
				ahead.byteVersion = 5;// SOCKS5
				ahead.byteAuthMethodsCount = 1;
				if(this->m_thisLogin.empty() && this->m_thisPassword.empty())
					ahead.byteMethods[0] = 0;//no auth
				else
					ahead.byteMethods[0] = 2;//auth with login and password

#if defined(SOCKS5_LOG)
				printf("[CProxy::Start]: Authentication...\n");
#endif

				if (this->m_Handler)
				{
					this->m_Handler(this, eSocks5Error::eProcessAuthentication);
				}

				if (this->Send(&ahead, sizeof(AuthRequestHeader)))
				{
					this->m_ProxyStatus = eProxyStatus::eSendAuthRequestHeader;
					return;
				}

				break;
			}
			case eProxyStatus::eSendAuthRequestHeader:
			{

				/*
					+----+--------+
					|VER | METHOD |
					+----+--------+
					| 1  |   1    |
					+----+--------+
				*/
				AuthRespondHeader arhead{};
				if (this->Receive(&arhead, sizeof(AuthRespondHeader)))
				{
					if (!this->m_thisLogin.empty() && !this->m_thisPassword.empty()) // auth
					{
						if (arhead.byteVersion != 5 || arhead.byteAuthMethod != 2)
						{
#if defined(SOCKS5_LOG)
							printf("[CProxy::Start->Error]: Authentication error. Invalid version or method -> ver: %d, method: %d\n", arhead.byteVersion, arhead.byteAuthMethod);
#endif
							closesocket(m_sockTCP);
							if (this->m_Handler)
							{
								this->m_Handler(this, eSocks5Error::eAuthenticationError);
							}
							this->m_bIsProcessing = false;
							this->m_ProxyStatus = eProxyStatus::eFailed;
							return;
						}
						else
						{
#if defined(SOCKS5_LOG)
							printf("[CProxy::Start]: Authentication was completed successfully.\n");
#endif
							if (this->m_Handler)
							{
								this->m_Handler(this, eSocks5Error::eAuthenticationSuccessful);
							}
							this->m_ProxyStatus = eProxyStatus::eSendAuthRequestPasswd;
							return;
						}
					}
					else if (this->m_thisLogin.empty() && this->m_thisPassword.empty()) // no auth
					{
						if (arhead.byteVersion != 5 || arhead.byteAuthMethod != 0)
						{
#if defined(SOCKS5_LOG)
							printf("[CProxy::Start->Error]: Authentication error. Invalid version or method -> ver: %d, method: %d\n", arhead.byteVersion, arhead.byteAuthMethod);
#endif
							closesocket(m_sockTCP);
							if (this->m_Handler)
							{
								this->m_Handler(this, eSocks5Error::eAuthenticationError);
							}
							this->m_bIsProcessing = false;
							this->m_ProxyStatus = eProxyStatus::eFailed;
							return;
						}
						else
						{
#if defined(SOCKS5_LOG)
							printf("[CProxy::Start]: Authentication was completed successfully.\n");
#endif
							if (this->m_Handler)
							{
								this->m_Handler(this, eSocks5Error::eAuthenticationSuccessful);
							}
							this->m_ProxyStatus = eProxyStatus::eSendConnectRequestHeader;
							return;
						}
					}
				}
				break;
			}
			case eProxyStatus::eSendAuthRequestPasswd:
			{
				/*   username/password request looks like
					* +----+------+----------+------+----------+
					* |VER | ULEN |  UNAME   | PLEN |  PASSWD  |
					* +----+------+----------+------+----------+
					* | 1  |  1   | 1 to 255 |  1   | 1 to 255 |
					* +----+------+----------+------+----------+
				*/
				char AuthRequest[1024] = { 0 };
				std::uint16_t AuthRequestLen = 0;
				AuthRequest[AuthRequestLen] = 1; //VER
				AuthRequestLen++;

				AuthRequest[AuthRequestLen] = (std::uint8_t)m_thisLogin.size();//ULEN
				AuthRequestLen++;

				std::memcpy(AuthRequest + AuthRequestLen, m_thisLogin.data(), m_thisLogin.size()); //UNAME
				AuthRequestLen += (std::uint16_t)m_thisLogin.size();

				AuthRequest[AuthRequestLen] = (std::uint8_t)m_thisPassword.size();//PLEN
				AuthRequestLen++;

				std::memcpy(AuthRequest + AuthRequestLen, m_thisPassword.data(), m_thisPassword.size()); //PASSWD
				AuthRequestLen += (std::uint16_t)m_thisPassword.size();

#if defined(SOCKS5_LOG)
				printf("[CProxy::Start]: Authorization.\n");
#endif

				if (this->m_Handler)
				{
					this->m_Handler(this, eSocks5Error::eProcessAuthorization);
				}

				if (this->Send(&AuthRequest, AuthRequestLen))
				{
					this->m_ProxyStatus = eProxyStatus::eAuthRespondHeaderAuth;
					return;
				}
				break;
			}
			case eProxyStatus::eSendConnectRequestHeader:
			{

				/*
					+----+-----+-------+------+----------+----------+
					|VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
					+----+-----+-------+------+----------+----------+
					| 1  |  1  | X'00' |  1   | Variable |    2     |
					+----+-----+-------+------+----------+----------+
				*/
				ConnectRequestHeader head{};
				head.byteVersion = 5; //SOCKS5
				head.byteCommand = 3; // tcp connection = 1, tcp binding = 2,  udp = 3
				head.byteReserved = 0;
				head.byteAddressType = 1; // IPv4=1, domain name = 3, IPv6 = 4
				head.ulAddressIPv4 = 0;
				head.usPort = 0;

#if defined(SOCKS5_LOG)
				printf("[CProxy::Start]: Connection...\n");
#endif

				if (this->m_Handler)
				{
					this->m_Handler(this, eSocks5Error::eQueryingDataAboutARemoteUDPServer);
				}

				if (this->Send(&head, sizeof(ConnectRequestHeader)))
				{
					this->m_ProxyStatus = eProxyStatus::eConnectRespondHeader;
					return;
				}
				break;
			}
			case eProxyStatus::eAuthRespondHeaderAuth:
			{
				AuthRespondHeaderAuth arheada{};
				if (this->Receive(&arheada, sizeof(AuthRespondHeaderAuth)))
				{
					if (arheada.byteVersion != 1 || arheada.byteStatus != 0)
					{
#if defined(SOCKS5_LOG)
						printf("[CProxy::Start->Error]: Authorization error. Invalid version or status -> ver: %d, status: %d\n", arheada.byteVersion, arheada.byteStatus);
#endif
						closesocket(this->m_sockTCP);
						if (this->m_Handler)
						{
							this->m_Handler(this, eSocks5Error::eAuthorizationError);
						}
						this->m_bIsProcessing = false;
						this->m_ProxyStatus = eProxyStatus::eFailed;
						return;
					}
					else
					{
#if defined(SOCKS5_LOG)
						printf("[CProxy::Start]: Authorization was completed successfully.\n");
#endif
						if (this->m_Handler)
						{
							this->m_Handler(this, eSocks5Error::eAuthenticationSuccessful);
						}
						this->m_ProxyStatus = eProxyStatus::eSendConnectRequestHeader;
						return;
					}
				}
				break;
			}
			case eProxyStatus::eConnectRespondHeader:
			{
				ConnectRespondHeader rhead{};
				if (this->Receive(&rhead, sizeof(ConnectRespondHeader)))
				{
					if (rhead.byteVersion != 5 || rhead.byteResult != 0)
					{
#if defined(SOCKS5_LOG)
						printf("[CProxy::Start->Error]: Connection error. Invalid version or result -> ver: %d, result: %d\n", rhead.byteVersion, rhead.byteResult);
#endif
						closesocket(m_sockTCP);
						if (this->m_Handler)
						{
							this->m_Handler(this, eSocks5Error::eFailedToQueryingDataAboutARemoteUDPServer);
						}
						this->m_bIsProcessing = false;
						this->m_ProxyStatus = eProxyStatus::eFailed;
						return;
					}
					else
					{
#if defined(SOCKS5_LOG)
						printf("[CProxy::Start]: Connected.\n");
#endif
						if (this->m_Handler)
						{
							this->m_Handler(this, eSocks5Error::eRemoteUDPServerDataSuccessfullyReceived);
						}
						m_proxyServerAddr.sin_family = AF_INET;
						m_proxyServerAddr.sin_port = rhead.usPort;
						m_proxyServerAddr.sin_addr.s_addr = rhead.ulAddressIPv4;

#if defined(SOCKS5_LOG)
						printf("[CProxy::Start]: Initializing a network address...\n");
#endif
						if (m_proxyServerAddr.sin_port == 0 || m_proxyServerAddr.sin_addr.s_addr == 0)
						{
							m_bIsStarted = false;
#if defined(SOCKS5_LOG)
							printf("[CProxy::Start]: Network address or port could not be initialized.\n");
#endif
							if (this->m_Handler)
							{
								this->m_Handler(this, eSocks5Error::eNetworkAddressOrPortCouldNotBeInitialized);
							}
							this->m_bIsProcessing = false;
							this->m_ProxyStatus = eProxyStatus::eFailed;
							return;
						}
						else
						{
							m_bIsStarted = true;
#if defined(SOCKS5_LOG)
							printf("[CProxy::Start]: The network address has been successfully initialized.\n");
							printf("[CProxy::Start]: Proxy initialized successfully.\n");

#endif

							this->m_bIsProcessing = false;
							if (this->m_Handler)
							{
								this->m_Handler(this, eSocks5Error::eProxyInitializedSuccessfully);
							}
							this->m_ProxyStatus = eProxyStatus::eInitialized;
							return;
						}
					}

#if defined(SOCKS5_LOG)
					printf("[CProxy::Start->Error]: Unknown error.\n");
#endif
					closesocket(m_sockTCP);
					if (this->m_Handler)
					{
						this->m_Handler(this, eSocks5Error::eNone);
					}
					this->m_bIsProcessing = false;
					this->m_ProxyStatus = eProxyStatus::eFailed;
					return;
				}

				break;
			}
			case eProxyStatus::eInitialized:
				break;
			}
		}
		//auth + no auth
		auto Start(const std::string ProxyIP, const std::string ProxyPort, const std::string ProxyLogin = "", const std::string ProxyPassword = "") -> void
		{
			this->m_proxyIP = inet_addr(ProxyIP.c_str());
			this->m_proxyPort = htons(std::atoi(ProxyPort.c_str()));
			this->m_thisIP = ProxyIP;
			this->m_thisPORT = ProxyPort;
			this->m_thisLogin = ProxyLogin;
			this->m_thisPassword = ProxyPassword;
			this->m_bIsProcessing = true;

#if defined(SOCKS5_LOG)
			printf("[CProxy::Start]: Running a proxy for the host: %s:%s\n", this->m_thisIP.c_str(), this->m_thisPORT.c_str());
#endif

			if (this->m_sockTCP != INVALID_SOCKET)
				closesocket(this->m_sockTCP);

			if ((this->m_sockTCP = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Couldn't create socket. (Socket error: %d)\n", GetLastError());
#endif
				if (this->m_Handler)
				{
					this->m_Handler(this, eSocks5Error::eFailedToCreateSocket);
				}

				this->m_ProxyStatus = eProxyStatus::eFailed;
				return;
			}

#ifdef _WIN32
			u_long mode = 1;
			if (ioctlsocket(this->m_sockTCP, FIONBIO, &mode) != 0) 
#else
			int flags = fcntl(this->m_sockTCP, F_GETFL, 0);
			if (flags == -1 || fcntl(this->m_sockTCP, F_SETFL, flags | O_NONBLOCK) == -1) 
#endif
			{
				closesocket(this->m_sockTCP);
#if defined (SOCKS5_LOG)
				printf("[CProxy::Start->Error]: Couldn't enable non-blocking socket. (Socket error: %d)\n", GetLastError());
#endif
				if (this->m_Handler)
				{
					this->m_Handler(this, eSocks5Error::eFailedToEnableNonBlockingMode);
				}

				this->m_ProxyStatus = eProxyStatus::eFailed;
				return;
			}		

			sockaddr_in sa{};
#if defined(_WIN32)
			sa.sin_addr.S_un.S_addr = m_proxyIP;
#else
			sa.sin_addr.s_addr = m_proxyIP;
#endif
			sa.sin_family = AF_INET;
			sa.sin_port = m_proxyPort;

			int result = connect(this->m_sockTCP, (sockaddr*)&sa, sizeof(sa));
			if (result == INVALID_SOCKET) 
			{

				int error = GetLastSocketError();
#ifdef _WIN32
        		if (error != WSAEWOULDBLOCK)
#else
        		if (error != EINPROGRESS)
#endif
				{
					closesocket(this->m_sockTCP);
#if defined (SOCKS5_LOG)
					printf("[CProxy::Start->Error]: Couldn't connect to the server. (Socket error: %d)\n", GetLastError());
#endif
					if (this->m_Handler)
					{
						this->m_Handler(this, eSocks5Error::eFailedToConnectToTCPServer);
					}

					this->m_ProxyStatus = eProxyStatus::eFailed;
					return;
				}
			}
			this->m_LastConnectionCheck = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
			this->m_ProxyStatus = eProxyStatus::eProcessConnectToSocket;
		}
		//check proxy is proccessing at this moment
		auto IsProcessing() const -> bool
		{
			return this->m_bIsProcessing;
		}
		//send datagram
		auto SendTo(SOCKET socket, char* data, std::int32_t dataLength, std::int32_t flags, sockaddr_in* to, std::int32_t tolen) -> std::int32_t
		{
			const std::int32_t data_len = sizeof(UDPDatagramHeader) + dataLength;
			std::uint8_t* proxyData = new std::uint8_t[data_len];
			UDPDatagramHeader* udph = (UDPDatagramHeader*)proxyData;
			std::memcpy((void*)((std::uintptr_t)proxyData + sizeof(UDPDatagramHeader)), data, dataLength);

			udph->usReserved = 0;
			udph->byteFragment = 0;
			udph->byteAddressType = 1;
#if defined(_WIN32)
			udph->ulAddressIPv4 = to->sin_addr.S_un.S_addr;
#else
			udph->ulAddressIPv4 = to->sin_addr.s_addr;
#endif
			udph->usPort = to->sin_port;

			auto len = sendto(socket, (char*)proxyData, data_len, 0, (const sockaddr*)&m_proxyServerAddr, sizeof(sockaddr_in));
			delete[] proxyData;

			return len;
		};
		//recv datagram
		int RecvFrom(SOCKET socket, char* buffer, std::int32_t bufferLength, std::int32_t flags, sockaddr_in* from, std::int32_t* fromlen) {
			const std::int32_t udphsize = sizeof(UDPDatagramHeader);
			char* data = new char[bufferLength + udphsize];
#if defined(_WIN32)
			auto len = recvfrom(socket, data, bufferLength + udphsize, flags, (sockaddr*)from, fromlen);
#else
			auto len = recvfrom(socket, data, bufferLength + udphsize, flags, (sockaddr*)from, (socklen_t*)fromlen);
#endif
			if (len <= 0)
			{
				delete[] data;
				return 0;
			}
			if (len <= udphsize)
			{
				std::memcpy(buffer, data, len);
				delete[] data;
				return len;
			}
			UDPDatagramHeader* udph = (UDPDatagramHeader*)data;
#if defined(_WIN32)
			from->sin_addr.S_un.S_addr = udph->ulAddressIPv4;
#else
			from->sin_addr.s_addr = udph->ulAddressIPv4;
#endif
			from->sin_port = udph->usPort;
			std::memcpy(buffer, (void*)((std::uintptr_t)data + udphsize), len - udphsize);
			delete[] data;
			return len - udphsize;
		};
		//check if proxy started
		auto IsStarted(void) const -> bool
		{
			return m_bIsStarted;
		};
		//restart connection to proxy 
		auto Restart(void) -> void
		{
			if (m_proxyIP == 0 || m_proxyPort == 0)
			{
#if defined(SOCKS5_LOG)
				printf("[CProxy::Restart->Error]: The proxy has not started yet. Restart is not possible.\n");
#endif
			}
			else
			{
				this->Start(this->m_thisIP, this->m_thisPORT, this->m_thisLogin, this->m_thisPassword);
			}
		};
		//get proxy ip
		auto GetProxyIP(void) const -> std::string
		{
			return m_thisIP;
		};
		//get proxy port
		auto GetProxyPort(void) const -> std::string
		{
			return m_thisPORT;
		};
		//get public proxy ip
		auto GetPublicProxyIP(void) const -> std::string
		{
			if (m_bIsStarted)
				return inet_ntoa(m_proxyServerAddr.sin_addr);
			else
				return "";
		};
		//use this if you successfully connected to server from proxy
		auto SetValidProxy(bool status) -> void
		{
			m_bIsValidReceiving = status;
		}
		//check if successfully connect to the server from proxy
		auto IsValidProxy(void) const -> bool
		{
			return m_bIsValidReceiving;
		}
		//set true if you want to redirect traffic from proxy
		auto SetReceivingByProxy(bool status) -> void
		{
			m_bIsReceivingByProxy = status;
		};
		//check if traffic redirecting from proxy
		auto IsReceivingByProxy(void) const -> bool
		{
			return m_bIsReceivingByProxy;
		}
		//disconnect from proxy
		auto Shutdown(void) -> void
		{
			if (m_sockTCP) {
				closesocket(m_sockTCP);
				m_bIsStarted = false;
			}
		}
	};
};

#endif // __SOCKS5_HPP