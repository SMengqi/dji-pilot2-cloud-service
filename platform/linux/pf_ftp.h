#ifndef _PF_FTP_UPIMAGES_H
#define _PF_FTP_UPIMAGES_H
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <errno.h>
#include <assert.h>
#include <netinet/in.h>
#include <dirent.h>
#include <ctype.h>

#define NOT_LONE_DASH(s) ((s)[0] != '-' || (s)[1])

#define FTPGETPUT_OPT_CONTINUE    1
#define FTPGETPUT_OPT_VERBOSE    2
#define FTPGETPUT_OPT_USER    4
#define FTPGETPUT_OPT_PASSWORD    8
#define FTPGETPUT_OPT_PORT    16


#define OFF_FMT "l"


/*#define LSA_SIZEOF_SA sizeof(           \
        union {                         \
            struct sockaddr sa;         \
            struct sockaddr_in sin;     \
        }                               \
    )                                   \
*/

typedef struct    ftpmissions{
    int currmission_id;
    int len_missionlist;
}ftpmissions_t;
typedef struct    ftpbackupmission_info{
    int currfilelist_id;
    int len_filelist;
}ftpbackupmission_info_t;


typedef struct len_and_sockaddr {
    socklen_t len;
    union {
        struct sockaddr sa;
        struct sockaddr_in sin;
    };
} len_and_sockaddr;

typedef struct ftp_host_info_s {
    const char *user;
    const char *password;
    struct len_and_sockaddr *lsa;
} ftp_host_info_t;

int xatou(char *buf);
int xatoul_range(char *buf, int low, int top);
len_and_sockaddr *xhost2sockaddr(const char *ip_addr, int port);
char* sockaddr2str(const struct sockaddr *sa, int flags);
char* xmalloc_sockaddr2dotted(const struct sockaddr *sa);
int xopen3(const char *pathname, int flags, int mode);
int xopen(const char *pathname, int flags);
ssize_t safe_read(int fd, void *buf, size_t count);
ssize_t safe_write(int fd, const void *buf, size_t count);
size_t full_write(int fd, const void *buf, size_t len);
off_t bb_full_fd_action(int src_fd, int dst_fd, off_t size);
off_t bb_copyfd_eof(int fd1, int fd2);
void ftp_die(const char *msg, const char *remote);
int ftpcmd(const char *s1, const char *s2, FILE *stream, char *buf);
void set_nport(len_and_sockaddr *lsa, unsigned port);
int xconnect_ftpdata(ftp_host_info_t *server, char *buf);
void xconnect(int s, const struct sockaddr *s_addr, socklen_t addrlen);
int xsocket(int domain, int type, int protocol);
int xconnect_stream(const len_and_sockaddr *lsa);
FILE *ftp_login(ftp_host_info_t *server, S32* pslRes);
int ftp_send(ftp_host_info_t *server, FILE *control_stream,
        const char *server_path, char *local_path);
int ftp_quit(FILE *control_stream);

#ifdef __cplusplus
extern "C" {
#endif

S32 pf_get_ftp_file_pasv_info(
        S8 *filename, 
        S8 *pcSaveFile, 
        S8* pscFtpUserName, 
        S8* pscFtpPassword,
        S8* pscFtpIpAddr,
        U32 ulFtpPort); 

S32 pf_put_ftp_file_pasv_info(
        S8 *filename, 
        S8 *pcSaveFile,
        S8* pscFtpUserName, 
        S8* pscFtpPassword,
        S8* pscFtpIpAddr,
        U32 ulFtpPort);

#ifdef __cplusplus
}
#endif


#endif //_PF_FTP_UPIMAGES_H

