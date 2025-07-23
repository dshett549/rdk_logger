#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "rdk_dynamic_logger.h"

#define COMP_SIGNATURE "LOG.RDK."
#define COMP_SIGNATURE_LEN 8
#define DL_PORT 12035
#define DL_SIGNATURE "COMC"
#define DL_SIGNATURE_LEN 4

static void usage(const char* app_name)
{
    printf("usage: %s <app_name> <module_name> <loglevel> \n",app_name);
    printf("Parameters:\n");
    printf("app_name    -> Application name, as per listed by 'ps' command\n");
    printf("module_name -> Module name.\n");
    printf("               For RDK component, the 'Module name' is expected to start with 'LOG.RDK.' string\n");
    printf("               For CPC component like 'Reciever', the module name can be either 'LOG.RDK.' or 'XREConnection', 'RmfMediaPlayer', etc.\n");
    printf("loglevel    -> Log Level of the Component to be modified\n");
    printf("               Possible values - FATAL,ERROR,WARN,NOTICE,INFO,DEBUG,TRACE\n");
    printf("               Turn off any loglevel using '~' symbol.\n");
    printf("               (i.e) '~ERROR' would turn off error logs alone for that component\n");
}

static char validate_loglevel(const char* level)
{
    char *loglevel = (char *)level;
    char negate = 0;

    if(loglevel[0] == '~') {
        loglevel++;
        negate = 0x80;
    }

    if(0 == strncmp(loglevel,"FATAL",5))
        return negate|RDK_LOG_FATAL;
    else if(0 == strncmp(loglevel,"ERROR",5))
        return negate|RDK_LOG_ERROR;
    else if(0 == strncmp(loglevel,"WARN",4))
        return negate|RDK_LOG_WARN;
    else if(0 == strncmp(loglevel,"NOTICE",6))
        return negate|RDK_LOG_NOTICE;
    else if(0 == strncmp(loglevel,"INFO",4))
        return negate|RDK_LOG_INFO;
    else if(0 == strncmp(loglevel,"DEBUG",5))
        return negate|RDK_LOG_DEBUG;
    else if(0 == strncmp(loglevel,"TRACE",6))
        return negate|RDK_LOG_TRACE;
    else
        return ENUM_RDK_LOG_COUNT;
}

static int validate_module_name(const char *name)
{
    FILE *fp = NULL;
    char ch;
    char comp_name[128] = {0};
    int ret = -1;

    fp = fopen("/etc/debug.ini","r");
    if(NULL == fp)
        return ret;

    do {
        ch = fscanf(fp, "%s", comp_name);
        if(strstr(comp_name,name)) {
            ret = 0;
            break;
        }
    } while(ch == 1);

    fclose(fp);
    return ret;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in dest_addr;
    int i, sockfd, numbytes, addr_len, app_len, comp_len, optval = 1;
    char level = -1;
    unsigned char buf[128] = {0};

    if (argc != 4) {
        usage(argv[0]);
        return -1;
    }

    if(0 != strcmp("Receiver",argv[1])) {
        if( (0 != strncmp(argv[2],COMP_SIGNATURE,COMP_SIGNATURE_LEN)) ||
                (0 != strncmp(argv[2], "LOG.RDK.", 8))) {
            printf("Invalid module name\n");
            usage(argv[0]);
            return -1;
        }
    }

    level = validate_loglevel(argv[3]);
    if(ENUM_RDK_LOG_COUNT == level) {
        printf("Invalid log level\n");
        usage(argv[0]);
        return -1;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        printf("socket: %s\n",strerror(errno));
        return -1;
    }

    if(setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,(char *) &optval,sizeof(optval))){
        printf("Error setting socket to BROADCAST mode %s\n",strerror(errno));
        return -1;
    }

    memset(&dest_addr,0,sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port=htons((unsigned short) DL_PORT);
    dest_addr.sin_addr.s_addr=inet_addr("127.255.255.255");

    /* Dynamic log signature 'COMC' */
    i = DL_SIGNATURE_LEN;
    memcpy(buf,DL_SIGNATURE,i);

    /* Log level */
    buf[++i] = (unsigned char)level;

    /* App name length */
    app_len = strlen(argv[1]);
    buf[++i] = app_len;

    /* App name */
    memcpy(buf+(++i),argv[1],app_len);

    /* Module name length */
    comp_len = strlen(argv[2]);
    i +=  app_len;
    buf[i] = comp_len;

    /* Module name */
    memcpy(buf+(++i),argv[2],comp_len);

    /* Total packet length excluding Dynamic log signature.
     * Application name length + the byte to hold that value +
     * Component name length + the byte to hold that value +
     * log level byte
     */
    buf[4] = app_len + comp_len + 3;

    /* Total packet length including Dynamic log signature +
     * the byte to hold that value
     */

#define DL_PACKET_LEN buf[4]+DL_SIGNATURE_LEN+1

    if ((numbytes=sendto(sockfd, buf, DL_PACKET_LEN, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr))) == -1) {
        printf("sendto: %s\n",strerror(errno));
        return -1;
    }

    printf( "Sent message to update log level of %s for %s process\n", argv[2], argv[1]);
    close(sockfd);
    return 0;
}

