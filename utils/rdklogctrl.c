#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <dirent.h>
#include "rdk_dynamic_logger.h"

#define COMP_SIGNATURE "LOG.RDK."
#define COMP_SIGNATURE_LEN 8
#define DL_SOCKET_DIR "/run/rdk_logger"
#define DL_SIGNATURE "COMC"
#define DL_SIGNATURE_LEN 4

static void usage(const char* app_name)
{
    printf("usage: %s <app_name> <module_name> <loglevel> \n",app_name);
    printf("Parameters:\n");
    printf("app_name    -> Application name, as per listed by 'ps' command\n");
    printf("module_name -> Module name.\n");
    printf("               For RDK component, the 'Module name' is expected to start with 'LOG.RDK.' string\n");
    printf("               For CPC component like 'Receiver', the module name can be either 'LOG.RDK.' or 'XREConnection', 'RmfMediaPlayer', etc.\n");
    printf("loglevel    -> Log Level of the Component to be modified\n");
    printf("               Possible values - FATAL, ERROR, WARN, NOTICE, INFO, DEBUG, TRACE, NONE\n");
}

static int8_t validate_loglevel(const char* level)
{
    char *loglevel = (char *)level;

    if (!level)
        return -1;

    if(0 == strncmp(loglevel, "FATAL", 5))
        return RDK_LOG_FATAL;
    else if(0 == strncmp(loglevel, "ERROR", 5))
        return RDK_LOG_ERROR;
    else if(0 == strncmp(loglevel, "WARN", 4))
        return RDK_LOG_WARN;
    else if(0 == strncmp(loglevel, "NOTICE", 6))
        return RDK_LOG_NOTICE;
    else if(0 == strncmp(loglevel, "INFO", 4))
        return RDK_LOG_INFO;
    else if(0 == strncmp(loglevel, "DEBUG", 5))
        return RDK_LOG_DEBUG;
    else if(0 == strncmp(loglevel, "TRACE", 6))
        return RDK_LOG_TRACE;
    else if(0 == strncmp(loglevel, "NONE", 4))
        return RDK_LOG_NONE;
    else
        return -1;
}

int main(int argc, char *argv[])
{
    struct sockaddr_un dest_addr;
    struct dirent *entry;
    DIR *socket_dir;
    int i, sockfd, sent = 0;
    size_t app_len, comp_len;
    long process_id;
    char *suffix;
    int8_t level = -1;
    unsigned char buf[128] = {0};

    if (argc != 4) {
        usage(argv[0]);
        return -1;
    }

    if(0 != strcmp("Receiver",argv[1])) {
        if( (0 != strncmp(argv[2],COMP_SIGNATURE,COMP_SIGNATURE_LEN)))
        {
            printf("Invalid module name\n");
            usage(argv[0]);
            return -1;
        }
    }

    level = validate_loglevel(argv[3]);
    if(-1 == level) {
        printf("Invalid log level\n");
        usage(argv[0]);
        return -1;
    }

    app_len = strlen(argv[1]);
    comp_len = strlen(argv[2]);
    if(app_len == 0 || app_len > sizeof(buf) - (DL_SIGNATURE_LEN + 4) ||
            comp_len > sizeof(buf) - (DL_SIGNATURE_LEN + 4) - app_len) {
        fprintf(stderr, "App and module names exceed packet size or app name is empty\n");
        return -1;
    }
    if(geteuid() != 0) {
        fprintf(stderr, "rdklogctrl requires root privileges\n");
        return -1;
    }

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        printf("socket: %s\n",strerror(errno));
        return -1;
    }

    memset(&dest_addr,0,sizeof(dest_addr));
    dest_addr.sun_family = AF_UNIX;

    /* Dynamic log signature 'COMC' */
    i = DL_SIGNATURE_LEN;
    memcpy(buf,DL_SIGNATURE,i);

    /* Log level */
    buf[++i] = (uint8_t)level;

    /* App name length */
    buf[++i] = app_len;

    /* App name */
    memcpy(buf+(++i),argv[1],app_len);

    /* Module name length */
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

    socket_dir = opendir(DL_SOCKET_DIR);
    if(socket_dir == NULL) {
        fprintf(stderr, "opendir: %s\n",strerror(errno));
        close(sockfd);
        return -1;
    }

    while((entry = readdir(socket_dir)) != NULL) {
        process_id = strtol(entry->d_name, &suffix, 10);
        if(process_id <= 0 || strcmp(suffix, ".sock") != 0)
            continue;
        if(snprintf(dest_addr.sun_path, sizeof(dest_addr.sun_path),
                DL_SOCKET_DIR "/%s", entry->d_name) >= (int)sizeof(dest_addr.sun_path))
            continue;
        if(sendto(sockfd, buf, DL_PACKET_LEN, 0,
                (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != -1)
            ++sent;
    }
    closedir(socket_dir);
    close(sockfd);
    if(sent == 0) {
        fprintf(stderr, "No dynamic logger sockets accepted the request\n");
        return -1;
    }

    printf( "Sent message to update log level of %s for %s process\n", argv[2], argv[1]);
    return 0;
}

