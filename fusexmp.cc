/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall fusexmp.c `pkg-config fuse --cflags --libs` -o fusexmp
*/

#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif
#include <stdlib.h>
#include <stdio.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <pthread.h>
#include "nfs_data.pb.h"

using namespace std;
char ipaddr[20];
int portnum;

  	


int Init_socket(){
	int client_sockfd; 
	int client_len;
	struct sockaddr_in clientaddr;
	client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
   	clientaddr.sin_family = AF_INET;
    	clientaddr.sin_addr.s_addr = inet_addr(ipaddr);
    	clientaddr.sin_port = htons(portnum);
	client_len = sizeof(clientaddr);

    	if (connect(client_sockfd, (struct sockaddr *)&clientaddr, client_len) < 0)
    	{
        	perror("Connect error: ");
        	exit(0);
    	}	
   	return client_sockfd;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int client_sockfd = Init_socket();
	int res;
	char buf_get[BUFSIZ];
	nfs::CData cdata;
	string data;
	string p = (string)path;
	cdata.set_call(2);	
	cdata.set_path(p);
	cdata.set_flags(fi->flags);
	cdata.SerializeToString(&data);
	printf("open : %s \n", path);
	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		return res;
	}
	
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		return res;
	}
	nfs::Result result;
	result.ParseFromString(buf_get);
	res = result.res();
		
	fi->fh = res;
	close(client_sockfd);
	printf("open2 \n");
	return 0;
}

static int xmp_getattr(const char *path, struct stat *statbuf)
{
	int client_sockfd = Init_socket();
	int res;
	char buf_get[500];
	nfs::CData cdata;
	string data;
	string p = (string)path;
	cdata.set_call(1);	
	cdata.set_path(p);
	printf("getattr: %s\n", path);
	cdata.SerializeToString(&data);

	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	
	res = recv(client_sockfd, buf_get, 500, 0);
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	
	nfs::Stat nstat;
	nstat.ParseFromString(buf_get);
	res = nstat.res();
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	
	statbuf->st_dev = nstat.dev();
	statbuf->st_ino = nstat.ino();
	statbuf->st_mode = nstat.mode();
	statbuf->st_nlink = nstat.nlink();
	statbuf->st_uid = nstat.uid();
	statbuf->st_gid = nstat.gid();
	statbuf->st_rdev = nstat.rdev();
	statbuf->st_size = nstat.size();
	statbuf->st_blksize = nstat.blksize();
	statbuf->st_blocks = nstat.blocks();
	statbuf->st_mtime = nstat.mtime();
	statbuf->st_ctime =nstat.ctime();
	statbuf->st_atime = nstat.atime();
	close(client_sockfd);
	return res;
}

static int xmp_access(const char *path, int mask)
{
	int client_sockfd = Init_socket();
	int res;
	char buf_get[BUFSIZ];
	nfs::CData cdata;
	string data;
	string p = (string)path;
	cdata.set_call(3);	
	cdata.set_path(p);
	cdata.set_mask(mask);
	cdata.SerializeToString(&data);

	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	nfs::Result result;
	result.ParseFromString(buf_get);
	res = result.res();
	close(client_sockfd);
	return res;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int client_sockfd = Init_socket();
	int res;
	char buf_get[BUFSIZ];
	nfs::CData cdata;
	string data;
	string p = (string)path;
	cdata.set_call(5);	
	cdata.set_path(p);
	cdata.set_size(size-1);
	cdata.SerializeToString(&data);
	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	nfs::Buffer buffer;
	res = buffer.res();
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	buffer.ParseFromString(buf_get);
	memcpy(buf, (char*)buffer.buf().c_str(), buffer.buf().length());
	close(client_sockfd);
	return res;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	int client_sockfd = Init_socket();
	int res;
	char buf_get[BUFSIZ];
	nfs::CData cdata;
	string data;
	string p = (string)path;
	cdata.set_call(4);	
	cdata.set_path(p);
	cdata.SerializeToString(&data);

	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	nfs::Filelist list;
	list.ParseFromString(buf_get);
	res = list.res();
	if(res < 0){
		close(client_sockfd);
		return res;
	}

	for(int i = 0; i < list.filename_size(); i++){
		filler(buf, list.filename(i).c_str(), NULL, 0);
	}
	close(client_sockfd);
	return res;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int client_sockfd = Init_socket();
	int res;
	char buf_get[BUFSIZ];
	nfs::CData cdata;
	string data;
	string p = (string)path;
	cdata.set_call(6);	
	cdata.set_path(p);
	cdata.set_mode(33204);
	cdata.set_rdev(rdev);
	
	
	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		cdata.set_flags(1);
	} else if (S_ISFIFO(mode))
		cdata.set_flags(2);
	else
		cdata.set_flags(3);
	cdata.SerializeToString(&data);

	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	nfs::Result result;
	result.ParseFromString(buf_get);
	res = result.res();
	close(client_sockfd);
	return res;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int client_sockfd = Init_socket();
	int res;
	char buf_get[BUFSIZ];
	nfs::CData cdata;
	string data;
	string p = (string)path;
	cdata.set_call(7);	
	cdata.set_path(p);
	cdata.set_mode(mode);
	cdata.SerializeToString(&data);
	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		return res;
	}
	nfs::Result result;
	result.ParseFromString(buf_get);
	res = result.res();
	
	return res;

	close(client_sockfd);
	return 0;
}

static int xmp_unlink(const char *path)
{
	int client_sockfd = Init_socket();
	int res;

	char buf_get[BUFSIZ];
	nfs::CData cdata;
	string data;
	string p = (string)path;
	cdata.set_call(8);	
	cdata.set_path(p);
	cdata.SerializeToString(&data);
	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		close(client_sockfd);
		return res;
	}
	nfs::Result result;
	result.ParseFromString(buf_get);
	res = result.res();
	close(client_sockfd);
	return res;
}

static int xmp_rmdir(const char *path)
{
	int client_sockfd = Init_socket();
	int res;

	char buf_get[BUFSIZ];
	nfs::CData cdata;
	string data;
	string p = (string)path;
	cdata.set_call(9);	
	cdata.set_path(p);
	cdata.SerializeToString(&data);
	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	nfs::Result result;
	result.ParseFromString(buf_get);
	res = result.res();
	close(client_sockfd);
	return res;
}

static int xmp_symlink(const char *from, const char *to)
{
	int client_sockfd = Init_socket();
	int res;
	char buf_get[BUFSIZ];
	nfs::CData cdata;
	string data;
	string fp = (string)from;
	string tp = (string)to;
	cdata.set_call(10);	
	cdata.set_frompath(fp);
	cdata.set_topath(tp);
	cdata.SerializeToString(&data);
	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	nfs::Result result;
	result.ParseFromString(buf_get);
	res = result.res();
	close(client_sockfd);
	return res;
}

static int xmp_rename(const char *from, const char *to)
{
	int client_sockfd = Init_socket();
	int res;
	char buf_get[BUFSIZ];
	nfs::CData cdata;
	string data;
	string fp = (string)from;
	string tp = (string)to;
	cdata.set_call(11);	
	cdata.set_frompath(fp);
	cdata.set_topath(tp);
	cdata.SerializeToString(&data);
	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	nfs::Result result;
	result.ParseFromString(buf_get);
	res = result.res();
	close(client_sockfd);
	return res;
}

static int xmp_link(const char *from, const char *to)
{
	int client_sockfd = Init_socket();
	int res;
	char buf_get[BUFSIZ];
	nfs::CData cdata;
	string data;
	string fp = (string)from;
	string tp = (string)to;
	cdata.set_call(12);	
	cdata.set_frompath(fp);
	cdata.set_topath(tp);
	cdata.SerializeToString(&data);
	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	nfs::Result result;
	result.ParseFromString(buf_get);
	res = result.res();
	close(client_sockfd);
	return res;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	int client_sockfd = Init_socket();
	int res;

	char buf_get[BUFSIZ];
	nfs::CData cdata;
	string data;
	string p = (string)path;
	cdata.set_call(13);	
	cdata.set_path(p);
	cdata.set_mode(mode);
	cdata.SerializeToString(&data);
	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	nfs::Result result;
	result.ParseFromString(buf_get);
	res = result.res();
	close(client_sockfd);
	return res;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{ 
	int client_sockfd = Init_socket();
	int res;
	char buf_get[BUFSIZ];
	nfs::CData cdata;
	string data;
	string p = (string)path;
	cdata.set_call(14);	
	cdata.set_path(p);
	cdata.set_uid(uid);
	cdata.set_gid(gid);
	cdata.SerializeToString(&data);
	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	nfs::Result result;
	result.ParseFromString(buf_get);
	res = result.res();
	close(client_sockfd);
	return res;
}

static int xmp_truncate(const char *path, off_t size)
{
	int client_sockfd = Init_socket();
	int res;
	char buf_get[BUFSIZ];
	nfs::CData cdata;
	string data;
	string p = (string)path;
	cdata.set_call(15);	
	cdata.set_path(p);
	cdata.set_size(size);
	cdata.SerializeToString(&data);
	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	nfs::Result result;
	result.ParseFromString(buf_get);
	res = result.res();
	printf("truncate: %s , %lld\n", path, size);
	close(client_sockfd);
	return res;
}

#ifdef HAVE_UTIMENSAT
static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int client_sockfd = Init_socket();
	int res;

	/* don't use utime/utimes since they follow symlinks */
	res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		return -errno;

	return 0;
}
#endif



static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int client_sockfd = Init_socket();
	int res;
	printf("read: %s \n", path);
	char buf_get[(int)size+1];
	nfs::CData cdata;
	string data;
	string p = (string)path;
	cdata.set_call(16);	
	cdata.set_path(p);
	cdata.set_size(size);
	cdata.set_offset(offset);
	cdata.SerializeToString(&data);
	printf("read: %s \n", path);
	res = send(client_sockfd, data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	res = recv(client_sockfd, buf_get, size, 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	nfs::Buffer buffer;
	res = buffer.res();
	if(res < 0){
		close(client_sockfd);return res;
	}
	buffer.ParseFromString(buf_get);
	//strcpy(buf, buffer.buf().c_str());
	cout<<buffer.buf()<<endl;
	memcpy(buf, buffer.buf().c_str(), buffer.buf().length());
	close(client_sockfd);
	return buffer.buf().length();
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int client_sockfd = Init_socket();
	int res;
	char buf_get[BUFSIZ];
	printf("write: %s \n", path);
	nfs::CData cdata;
	string data;
	string p = (string)path;
	cdata.set_call(17);	
	cdata.set_path(p);
	cdata.set_size(size);
	cdata.set_offset(offset);
	cdata.set_buf(buf);
	cdata.SerializeToString(&data);
	res = send(client_sockfd,data.c_str(), data.length(), 0);
	if(res < 0){
		close(client_sockfd);return res;
	}
	printf("write2: %s \n", path);
	res = recv(client_sockfd, buf_get, BUFSIZ, 0);
	if(res < 0){
		printf("write recv fail: %d \n", res);
		close(client_sockfd);return res;
	}
	printf("write3: %s \n", path);
	nfs::Result result;
	result.ParseFromString(buf_get);
	res = result.res();
	close(client_sockfd); 
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int xmp_fallocate(const char *path, int mode,
			off_t offset, off_t length, struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;

	if (mode)
		return -EOPNOTSUPP;

	fd = open(path, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = -posix_fallocate(fd, offset, length);

	close(fd);
	return res;
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

void *xmp_init(struct fuse_conn_info *conn)
{

}

void xmp_destroy(void *userdata)
{
   int client_sockfd = Init_socket();
   send(client_sockfd, "close", 5, 0);
   close(client_sockfd);
}

static struct fuse_operations xmp_oper;

void Init_oper(){
	xmp_oper.getattr	= xmp_getattr;
	xmp_oper.access		= xmp_access;
	xmp_oper.readlink	= xmp_readlink;
	xmp_oper.readdir	= xmp_readdir;
	xmp_oper.mknod		= xmp_mknod;
	xmp_oper.mkdir		= xmp_mkdir;
	xmp_oper.symlink	= xmp_symlink;
	xmp_oper.unlink		= xmp_unlink;
	xmp_oper.rmdir		= xmp_rmdir;
	xmp_oper.rename		= xmp_rename;
	xmp_oper.link		= xmp_link;
	xmp_oper.chmod		= xmp_chmod;
	xmp_oper.chown		= xmp_chown;
	xmp_oper.truncate	= xmp_truncate;
#ifdef HAVE_UTIMENSAT
	xmp_oper.utimens	= xmp_utimens;
#endif
	xmp_oper.open		= xmp_open;
	xmp_oper.read		= xmp_read;
	xmp_oper.write		= xmp_write;
	xmp_oper.statfs		= xmp_statfs;
	xmp_oper.release	= xmp_release;
	xmp_oper.fsync		= xmp_fsync;
#ifdef HAVE_POSIX_FALLOCATE
	xmp_oper.fallocate	= xmp_fallocate;
#endif
#ifdef HAVE_SETXATTR
	xmp_oper.setxattr	= xmp_setxattr;
	xmp_oper.getxattr	= xmp_getxattr;
	xmp_oper.listxattr	= xmp_listxattr;
	xmp_oper.removexattr	= xmp_removexattr;
#endif
	xmp_oper.init 		= xmp_init;
  	xmp_oper.destroy 	= xmp_destroy;
}

int main(int argc, char *argv[])
{
	strcpy(ipaddr,argv[argc-3]);
    	portnum = atoi(argv[argc-2]);
	argv[argc-3] = argv[argc-1];
    	argv[argc-2] = NULL;
    	argv[argc-1] = NULL;
	
	argc-=2;

	umask(0);
	//Init_socket();
	Init_oper();
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
