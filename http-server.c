#include <ctype.h>
#include <netdb.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

static void die(const char *message)
{
    perror(message);
    exit(1);
}

int main(int argc, char **argv) {


    if (argc != 5) {
        fprintf(stderr, "%s\n", "Usage: ./http-server <server_port> <web_root> <mdb-lookup-host> <mdb-lookup-port>");
        exit(1);
    }
    unsigned short port = atoi(argv[1]);

    int servsock;
    if ((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket failed");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(servsock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        die("bind failed");
    if (listen(servsock, 5) < 0)
        die("listen failed");
    
    unsigned short mdbport = atoi(argv[4]);
    int mdbsock;
    if ((mdbsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket failed");

    struct hostent *he;
    char *serverName = argv[3];
    if ((he = gethostbyname(serverName)) == NULL)
        die("gethostbyname failed");
    char *serverIP = inet_ntoa(*(struct in_addr *)he->h_addr);

    struct sockaddr_in mdbaddr;
    memset(&mdbaddr, 0, sizeof(mdbaddr));
    mdbaddr.sin_family = AF_INET;
    mdbaddr.sin_addr.s_addr = inet_addr(serverIP);
    mdbaddr.sin_port = htons(mdbport);

    if (connect(mdbsock, (struct sockaddr *) &mdbaddr, sizeof(mdbaddr)) < 0){
        die("connect failed");
    }
    
    FILE *results;
    int clntsock;
    socklen_t clntlen;
    struct sockaddr_in clntaddr;
    char aaa[1000];

    char message[1000];
    int once = 0;
    char skip[1000];

    char filepath[200];

    char key[17];
    char *bad_URI;
    char bad_URI1[4];
    char *add;
    char msg[4096];
    char r[1000];
    char buf[1000];
    char html[1000];
    while (1) {
        clntlen = sizeof(clntaddr);
        if ((clntsock = accept(servsock,(struct sockaddr *) &clntaddr, &clntlen)) < 0)
            die("accept failed");
        FILE *rec = fdopen(clntsock,"r");
        if (fgets(aaa,sizeof(aaa),rec) == NULL)
            continue;
        aaa[strlen(aaa)]='\0';
        snprintf(message,sizeof(message),"%s",aaa);

        char new_message[strlen(message)-1];
        snprintf(new_message,sizeof(new_message),"%s",message);

        char *token_separators = "\t \r\n"; // tab, space, new line
        char *method = strtok(aaa, token_separators);
        char *requestURI = strtok(NULL, token_separators);
        char *httpVersion = strtok(NULL, token_separators);

        if (!method || !requestURI || !httpVersion) {
            if (!requestURI)
                method = "(null)";
            if (!httpVersion)
                method = "(null)";
            if (aaa[0] == '\n'){

                fprintf(stderr,"%s \"%s %s %s\" 400 Bad Request\n",inet_ntoa(clntaddr.sin_addr), "(null)", requestURI, httpVersion);
            } else{
                fprintf(stderr,"%s \"%s %s %s\" 400 Bad Request\n",inet_ntoa(clntaddr.sin_addr), aaa, requestURI, httpVersion);
            }
            send(clntsock, "HTTP/1.0 400 Bad Request\r\n\r\n",strlen("HTTP/1.0 400 Bad Request\r\n\r\n"),0);
            send(clntsock, "<html><body>\r\n<h1>400 Bad Request</h1>\r\n</body></html>\r\n", strlen("<html><body>\r\n<h1>400 Bad Request</h1>\r\n</body></html>\r\n"),0);
            fclose(rec);
            close(clntsock);
            continue;
        }

        strncpy(key,requestURI,16);
        
        if (strcmp(method,"GET") != 0 || (strcmp(httpVersion, "HTTP/1.0") != 0 && strcmp(httpVersion, "HTTP/1.1") != 0)) {
            fprintf(stderr,"%s \"%s\" 501 Not Implemented\n",inet_ntoa(clntaddr.sin_addr), new_message);
            send(clntsock, "HTTP/1.0 501 Not Implemented\r\n\r\n",strlen("HTTP/1.0 501 Not Implemented\r\n\r\n"),0);
            send(clntsock, "<html><body>\r\n<h1>501 Not Implemented</h1>\r\n</body></html>\r\n", strlen("<html><body>\r\n<h1>501 Not Implemented</h1>\r\n</body></html>\r\n"),0);
            fclose(rec);
            close(clntsock);
            continue;
        }
        if (requestURI[0] != '/') {
            fprintf(stderr,"%s \"%s\" 400 Bad Request\n",inet_ntoa(clntaddr.sin_addr), new_message);
            send(clntsock,"400 Bad Request\r\n\r\n",strlen("400 Bad Request\r\n\r\n"),0);
            send(clntsock, "<html><body>\r\n<h1>400 Bad Request</h1>\r\n</body></html>\r\n",strlen("<html><body>\r\n<h1>400 Bad Request</h1>\r\n</body></html>\r\n"),0);
            fclose(rec);
            close(clntsock);
            continue;
        }
        bad_URI = strstr(message,"/../");

        strncpy(bad_URI1, message+(strlen(message)-3),3);
        if (bad_URI || strcmp(bad_URI1,"/..") == 0) {
            fprintf(stderr,"%s \"%s\" 400 Bad Request\n",inet_ntoa(clntaddr.sin_addr), new_message);
            send(clntsock,"400 Bad Request\r\n\r\n",strlen("400 Bad Request\r\n\r\n"),0);
            send(clntsock, "<html><body>\r\n<h1>400 Bad Request</h1>\r\n</body></html>\r\n",strlen("<html><body>\r\n<h1>400 Bad Request</h1>\r\n</body></html>\r\n"),0);
            fclose(rec);
            close(clntsock);
            continue;
        }
        while (strcmp(fgets(skip,sizeof(skip),rec),"\r\n") != 0);
        add = inet_ntoa(clntaddr.sin_addr);
        add[strlen(add)] = '\0';

        struct stat aah;
        snprintf(filepath,sizeof(filepath),"%s%s",argv[2],requestURI);
        stat(filepath,&aah);
        mode_t type = 0;
        type = aah.st_mode;

        if (strcmp(requestURI,"/mdb-lookup") != 0 && strcmp(requestURI,"/favicon.ico") != 0&& !strstr(requestURI,"/mdb-lookup?key")) {

            if (S_ISDIR(type)) {
                if (requestURI[strlen(requestURI)-1] != '/') {
                    fprintf(stderr,"%s \"%s\" 403 Forbidden\n",inet_ntoa(clntaddr.sin_addr), new_message);
                    send(clntsock,"403 Forbidden\r\n\r\n",strlen("403 Forbidden\r\n\r\n"),0);
                    send(clntsock,"<html><body>\r\n<h1>403 Forbidden</h1>\r\n</body></html>\r\n",strlen("<html><body>\r\n<h1>403 Forbidden</h1>\r\n</body></html>\r\n"),0);
                    fclose(rec);
                    close(clntsock);
                    continue;
                } else {

                    strcat(filepath,"index.html");
                }
            }
        }
        if (strcmp(requestURI,"/mdb-lookup") != 0 && !strstr(requestURI,"/mdb-lookup?key")){
            int sen;
            FILE *file = fopen(filepath, "rb");
            if(file) {  
                fprintf(stderr,"%s \"%s\" 200 OK\n",add, new_message);
            }
            else if (!file) {
                fprintf(stderr,"%s \"%s\" 404 Not Found\n",add, new_message);
                send(clntsock,"404 Not Found\r\n\r\n",strlen("404 Not Found\r\n\r\n"),0);
                send(clntsock,"<html><body>\r\n<h1>404 Not Found</h1>\r\n</body></html>\r\n",strlen("<html><body>\r\n<h1>404 Not Found</h1>\r\n</body></html>\r\n"),0);
                fclose(rec);
                close(clntsock);
                continue;
            }
            send(clntsock,"HTTP/1.0 200 OK\r\n\r\n",strlen("HTTP/1.0 200 OK\r\n\r\n"),0);
            while ((sen = fread(msg,1,sizeof(msg),file))) {
                if (send(clntsock,msg,sen,0) != sen ) {
                    fclose(file);
                    fclose(rec);
                    close(clntsock);
                    continue;
                }
            }
        } else {
            send(clntsock,"HTTP/1.0 200 OK\r\n\r\n",strlen("HTTP/1.0 200 OK\r\n\r\n"),0);
            if (once == 0 && strcmp(requestURI,"/mdb-lookup") == 0) {
                fprintf(stderr,"%s \"%s\" 200 OK\n",add, new_message);
                once++;
            }
            results = fdopen(mdbsock, "rb");
            if (strcmp("/mdb-lookup",requestURI) == 0) {

                const char *form =
                    "<html><body>\n"
                    "<h1>mdb-lookup</h1>\n"
                    "<p>\n"
                    "<form method=GET action=/mdb-lookup>\n"
                    "lookup: <input type=text name=key>\n"
                    "<input type=submit>\n"
                    "</form>\n"
                    "<p>\n"
                    "</body></html>\n";
                send(clntsock,form,strlen(form),0);
            } else if (strstr(requestURI,"/mdb-lookup?key=")) {

                const char *form =
                    "<html><body>\n"
                    "<h1>mdb-lookup</h1>\n"
                    "<p>\n"
                    "<form method=GET action=/mdb-lookup>\n"
                    "lookup: <input type=text name=key>\n"
                    "<input type=submit>\n"
                    "</form>\n"
                    "<p>\n";

                int yellow = 0;
                //send over the socket
                char *md = strstr(requestURI,"=");
                md = md + 1;
                snprintf(buf,sizeof(buf),"%s\n",md);

                send(mdbsock,buf,strlen(buf),0);
                send(clntsock,form,strlen(form),0);

                send(clntsock,"<p><table border>\n",strlen("<p><table border>\n"),0);
                fprintf(stderr,"looking up [%s]: %s \"%s\" 200 OK\n",md,serverIP,new_message);
                while(fgets(r,sizeof(r),results) != NULL && r[0] != '\n') {
                    if (yellow == 0) {
                        char *form = "<tr><td>";
                        snprintf(html,sizeof(html),"%s%s",form,r);
                        send(clntsock,html,strlen(html),0);
                        send(clntsock,"\n",strlen("\n"),0);
                        yellow = 1;
                    } else if (yellow == 1) {
                        char *form = "<tr><td bgcolor=yellow>";
                        snprintf(html,sizeof(html),"%s%s",form,r);
                        send(clntsock,html,strlen(html),0);
                        send(clntsock,"\n",strlen("\n"),0);
                        yellow = 0;
                    }
                }
                char *end =
                    "</table>\n"
                    "</body></html>\n";

                send(clntsock,end,strlen(end),0);
            }
        }

	fclose(rec);
	close(clntsock);
    }
    fclose(results);
    close(mdbsock);
    close(servsock);
}

