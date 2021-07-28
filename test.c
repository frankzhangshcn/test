/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2021, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
/* <DESC>
 * An example demonstrating how an application can pass in a custom
 * socket to libcurl to use. This example also handles the connect itself.
 * </DESC>
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>

#ifdef WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
#else
#include <sys/types.h>        /*  socket types              */
#include <sys/socket.h>       /*  socket definitions        */
#include <netinet/in.h>
#include <arpa/inet.h>        /*  inet (3) functions         */
#include <unistd.h>           /*  misc. Unix functions      */
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

#endif

#include <errno.h>

/* The IP address and port number to connect to */
#define IPADDR "14.215.177.38" //"127.0.0.1"

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    //printf("fz write\n");
	//return nmemb; // Frank Zhang
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

static int closecb(void *clientp, curl_socket_t item)
{
  (void)clientp;
  printf("fz libcurl wants to close %d now\n", (int)item);
  return 0;
}

static curl_socket_t opensocket(void *clientp,
                                curlsocktype purpose,
                                struct curl_sockaddr *address)
{
    printf("fz curl_socket_t opensocket\n");
  curl_socket_t sockfd;
  (void)purpose;
  (void)address;
  sockfd = *(curl_socket_t *)clientp;
  /* the actual externally set socket is passed in via the OPENSOCKETDATA
     option */
  return sockfd;
}

static int sockopt_callback(void *clientp, curl_socket_t curlfd,
                            curlsocktype purpose)
{
    printf("sockopt_callback\n");
  (void)clientp;
  (void)curlfd;
  (void)purpose;
  /* This return code was added in libcurl 7.21.5 */
  return CURL_SOCKOPT_ALREADY_CONNECTED;
}

#define HTTPC_RETRY_COUNT      (3)
#define ACCESS_TOKEN_LEN  (128)
//const char host[]="iot-ge.xlink.cloud";
//const char host[]="127.0.0.1";
//const char host[]="www.baidu.com";
//define XLINK_SERVER_ADDR           "cm-ge.xlink.cn"
#if 1
//const char host[]="cm-ge.xlink.cn";
//#define PORTNUM     (23779)
const char host[]="api-ge.xlink.cn";
#define PORTNUM     (443)
#define XLINK_OTA_ADDR                  "api-ge.xlink.cn"
#define XLINK_OTA_PORT                  (443)
#else
const char host[]="www.baidu.com";
#define PORTNUM      (80)
#endif

#define XLINK_SERVER_PORT           "23779"
const char product_id[]="1607d4c1275000011607d4c12750d806";
const char product_key[]="abc9bd4baaf4a3a9b17c7194f87cf5c8";
char access_token[ACCESS_TOKEN_LEN];
static curl_socket_t connect_server(void)
{
	char ip[128];
	curl_socket_t m_sock = -1;
    memset(ip, 0, sizeof(ip));
    strcpy(ip, host);
 
	void* svraddr = NULL;
    int error=-1, svraddr_len;
    int ret = 0;
    struct sockaddr_in svraddr_4;
    struct sockaddr_in6 svraddr_6;
 
    //获取网络协议
    struct addrinfo *result;
    error = getaddrinfo(ip, NULL, NULL, &result);
    const struct sockaddr *sa = result->ai_addr;
    socklen_t maxlen = 128;
    switch(sa->sa_family) {
        case AF_INET://ipv4
			if ((m_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("socket create failed");
                ret = -1;
                break;
            }
            #if 0
            if(inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                         ip, maxlen) < 0){
                perror(ip);
                ret = -1;
                break;
            }
            #endif
            svraddr_4.sin_family = AF_INET;
            //svraddr_4.sin_addr.s_addr = inet_addr(ip);
            svraddr_4.sin_addr = ((struct sockaddr_in *)sa)->sin_addr;
            svraddr_4.sin_port = htons(PORTNUM);
            svraddr_len = sizeof(svraddr_4);
            svraddr = &svraddr_4;
            break;
        case AF_INET6://ipv6
            if ((m_sock = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
                perror("socket create failed");
                ret = -1;
                break;
            }
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                      ip, maxlen);
            
            printf("socket created ipv6/n");
            
            bzero(&svraddr_6, sizeof(svraddr_6));
            svraddr_6.sin6_family = AF_INET6;
            svraddr_6.sin6_port = htons(PORTNUM);
            if ( inet_pton(AF_INET6, ip, &svraddr_6.sin6_addr) < 0 ) {
                perror(ip);
                ret = -1;
                break;
            }
            svraddr_len = sizeof(svraddr_6);
            svraddr = &svraddr_6;
            break;
            
        default:
            printf("Unknown AF\ns");
			ret = -1;
    }
	freeaddrinfo(result);
	if(ret < 0)
	{
		fprintf(stderr , "Cannot Connect the server!n");
		printf("failed\n");
		if(m_sock>=0)
		{
			close(m_sock);
			m_sock = -1;
		}
		return m_sock;
	}
	int nret = connect(m_sock, (struct sockaddr*)svraddr, svraddr_len);
	if(nret==-1 )
    {
		printf("failed\n");
		if(m_sock>=0)
		{
			close(m_sock);
			m_sock = -1;
		}
		return m_sock;	
	}
	return m_sock;
}
/* resizable buffer */
typedef struct {
  char *buf;
  size_t size;
} memory;
size_t grow_buffer(void *contents, size_t sz, size_t nmemb, void *ctx)
{
  size_t realsize = sz * nmemb;
  memory *mem = (memory*) ctx;
  char *ptr = realloc(mem->buf, mem->size + realsize);
  if(!ptr) {
    /* out of memory */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
  mem->buf = ptr;
  memcpy(&(mem->buf[mem->size]), contents, realsize);
  mem->size += realsize;
  //printf("data:%d\n",realsize);
  return realsize;
}
static int get_json_string(char *json, char *string, char *data, int len)
{
	int ret = -1, temp_len = 0;
	char *temp_str1 = NULL, *temp_str2 = NULL;
    if(!json || !string || len <= 0)
	{
        printf("get_json_string param error");
		return ret;
	}
	
	temp_str1 = strstr(json, string);
	if(temp_str1 != NULL)
	{
		temp_str1 += 1;
		temp_str1 = strstr(temp_str1, ":");		
		if(temp_str1 != NULL)
		{
            temp_str1 += 1;
			temp_str1 = strstr(temp_str1, "\"");			
			if(temp_str1 != NULL)
			{
                temp_str1 += 1;
				temp_str2 = strstr(temp_str1, "\"");
			}
		}
	}
	if((temp_str1 != NULL) && (temp_str2 != NULL))
	{
        temp_len = temp_str2 - temp_str1;
		if(temp_len <= len)
		{
			memcpy(data, temp_str1, temp_len);
            data[temp_len] = 0;
			ret = 0;
		}
        else
        {
            printf("%s is too long", string);
        }
	}

	return ret;
}
int get_access_token(char *access_token)
{
	int ret = -1;
	int retry_count = 0;
	//struct httpc_conn *conn = NULL;
    uint8_t temp_mac[6] = {0};
    uint8_t mac[13] = {0};    
	uint8_t post_data[512] = {0};
	uint8_t *buf = NULL;
	int read_size = 0, total_size = 0;
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    memory *mem = malloc(sizeof(memory));
    if(!mem) {
        /* out of memory */
        printf("not enough memory (realloc returned NULL)\n");
        return ret;
    }
    mem->size = 0;  
    mem->buf = malloc(1);    
    if(!mem->buf) {
        /* out of memory */
        printf("not enough memory (realloc returned NULL)\n");
        return ret;
    }
    retry_count = 1;//HTTPC_RETRY_COUNT;
    while(retry_count > 0)
	{
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(curl) {
        char nline[256];        
        curl_socket_t sockfd;
        sockfd = connect_server();
	    if(sockfd < 0)	
        {
          printf("client error: connect: %s\n", strerror(errno));
          curl_easy_cleanup(curl);
          goto error;
        }
        //XlinkGetAuthCode(auth_code);
        temp_mac[0] = 0x78;
        temp_mac[1] = 0x6D;
        temp_mac[2] = 0xE8;
        temp_mac[3] = 0x33;
        temp_mac[4] = 0x6B;
        temp_mac[5] = 0x2C;
        char auth_code[] = "1e07d4c1ff525001";

        snprintf(mac, sizeof(mac), "%02X%02X%02X%02X%02X%02X", temp_mac[0], temp_mac[1], temp_mac[2], temp_mac[3], temp_mac[4], temp_mac[5]);
        
        sprintf(post_data, "{\"product_id\":\"%s\",\"mac\":\"%s\",\"authorize_code\":\"%s\"}", product_id, mac, auth_code);
        //printf("post_data:%s\r\n", post_data);
    
        //设置url
        //curl_easy_setopt(curl, CURLOPT_URL, host);
        //curl_easy_setopt(curl, CURLOPT_URL, "https://api-ge.xlink.cn");
        curl_easy_setopt(curl, CURLOPT_URL, "https://api-ge.xlink.cn/v2/device_login");
        curl_easy_setopt(curl, CURLOPT_PORT, PORTNUM);
        /* no progress meter please */
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
#if 1

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

        /* call this function to close sockets */
        //curl_easy_setopt(curl, CURLOPT_CLOSESOCKETFUNCTION, closecb);
        //curl_easy_setopt(curl, CURLOPT_CLOSESOCKETDATA, &sockfd);
        /* call this function to set options for the socket */
        //curl_easy_setopt(curl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, grow_buffer);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, mem);
        headers = curl_slist_append(headers, "Content-Type:application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        //curl_easy_setopt(curl, CURLOPT_REQUEST_TARGET, "/v2/device_login");
        //curl_easy_setopt(curl,  CURLOPT_CUSTOMREQUEST, "POST /v2/device_login");
        res = curl_easy_perform(curl);
#else
        //构建HTTP报文头  
        //sprintf_s(tmp_str, "Content-Length: %s", jsonout.c_str());
        //headers = curl_slist_append(headers, "Content-Length:application/json;charset=UTF-8");
        headers = curl_slist_append(headers, "Content-Type:application/json");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        //curl_easy_setopt(curl,  CURLOPT_CUSTOMREQUEST, "POST");//自定义请求方式
        //curl_easy_setopt(curl, CURLOPT_POST, 1);//设置为非0表示本次操作为POST
        // 设置要POST的JSON数据
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        //curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post_data));//设置上传json串长度,这个设置可以忽略
/* buffer body */
  
        // 设置接收数据的处理函数和存放变量
        //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);//设置回调函数
        //curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);//设置写数据
        
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, grow_buffer);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, mem);
        /* call this function to set options for the socket */
        curl_easy_setopt(curl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_REQUEST_TARGET, "/v2/device_login");
        
        res = curl_easy_perform(curl);//执行
#endif
        curl_slist_free_all(headers); /* free the list again */
        printf("data:%d:%s\n",mem->size,mem->buf);
        if((ret = get_json_string((char*)mem->buf, "\"access_token\"", access_token, ACCESS_TOKEN_LEN)) != 0)
        {
            printf("Get access_token failed\n");
        }
        printf("access_token:%s\n",access_token);
        //string str_json = out.str();//返回请求值 
        //printf("%s",str_json.c_str()); 
        /* no progress meter please */
        //curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

        /* send all data to this function  */
        //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

        /* call this function to get a socket */
        //curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, opensocket);
        //curl_easy_setopt(curl, CURLOPT_OPENSOCKETDATA, &sockfd);

        /* call this function to close sockets */
        //curl_easy_setopt(curl, CURLOPT_CLOSESOCKETFUNCTION, closecb);
        //curl_easy_setopt(curl, CURLOPT_CLOSESOCKETDATA, &sockfd);

        /* call this function to set options for the socket */
        //curl_easy_setopt(curl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);

        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        //sprintf(post_data, "{\"product_id\":\"%s\",\"mac\":\"%s\",\"authorize_code\":\"%s\"}", product_id, mac, "authtestcode");
    //printf("post_data:%s\r\n", post_data);
    
    //ret = httpc_request_write_header_start(conn, "POST", "/v2/device_login", "application/json", strlen(post_data));
    //headers = curl_slist_append(headers, "Accept:application/json");
 

        //headers = curl_slist_append(headers, "POST /v2/device_login");
        //headers = curl_slist_append(headers, "Content-Type:application/json");
        //curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        //curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        //res = curl_easy_perform(curl);

        curl_easy_cleanup(curl);

        close(sockfd);
    }
    error:        
        curl_global_cleanup();
        retry_count--;
        usleep(500000);
		//vTaskDelay(500);
    }
    return ret;
	
}
int main(void)
{
    char access_token[ACCESS_TOKEN_LEN];
    get_access_token(access_token);
    return 0;
  CURL *curl;
  CURLcode res;
  curl_socket_t sockfd;


curl_global_init(CURL_GLOBAL_ALL);

  curl = curl_easy_init();
  if(curl) {
    /*
     * Note that libcurl will internally think that you connect to the host
     * and port that you specify in the URL option.
     */
    //curl_easy_setopt(curl, CURLOPT_URL, "http://99.99.99.99:9999");
    curl_easy_setopt(curl, CURLOPT_URL, "www.baidu.com");
    sockfd = connect_server();
	if(sockfd < 0)	
    {
      printf("client error: connect: %s\n", strerror(errno));
      return 1;
    }

    /* no progress meter please */
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

    /* send all data to this function  */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

    /* call this function to get a socket */
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, opensocket);
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETDATA, &sockfd);

    /* call this function to close sockets */
    curl_easy_setopt(curl, CURLOPT_CLOSESOCKETFUNCTION, closecb);
    curl_easy_setopt(curl, CURLOPT_CLOSESOCKETDATA, &sockfd);

    /* call this function to set options for the socket */
    curl_easy_setopt(curl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

    res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    close(sockfd);

    if(res) {
      printf("libcurl error: %d\n", res);
      return 4;
    }
  }
curl_global_cleanup();
  return 0;
}

