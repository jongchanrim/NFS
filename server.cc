#include <stdlib.h>
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
#include "nfs_data.pb.h"
int read_from_socket(int  client_sockfd);
int server_sockfd, client_sockfd;
    int state;
    int pid, res;
    socklen_t client_len;
    struct sockaddr_in clientaddr, serveraddr;
using namespace std;
char serverdir[] = "/home/parallels/Documents/server";
//1
void get_attr(string path){
	string data;
	char fpath[1000];
	strcpy(fpath,serverdir);
	strcat(fpath, path.c_str());
	struct stat *statbuf;
	statbuf =(struct stat*) malloc(sizeof(struct stat));
	
	int res = lstat(fpath, statbuf);
	printf("getattr res: %d \n", res);
	printf("getattr path: %s \n", fpath);
	if( res < 0) {
		res = -errno;
	}
	
	nfs::Stat nstat;
	nstat.set_dev(statbuf->st_dev);
	nstat.set_ino(statbuf->st_ino);
	nstat.set_mode(statbuf->st_mode);
	nstat.set_nlink(statbuf->st_nlink);
	nstat.set_uid(statbuf->st_uid);
	nstat.set_gid(statbuf->st_gid);
	nstat.set_rdev(statbuf->st_rdev);
	nstat.set_size(statbuf->st_size);
	nstat.set_blksize(statbuf->st_blksize);
	nstat.set_blocks(statbuf->st_blocks);
	nstat.set_mtime(statbuf->st_mtime);
	nstat.set_ctime(statbuf->st_ctime);
	nstat.set_atime(statbuf->st_atime);
	nstat.set_res(res);
	nstat.SerializeToString(&data);
	send(client_sockfd, data.c_str(), data.length(), 0);
	free(statbuf);
	}

//2
void open_server(string path, nfs::CData cdata){
	string data;
	char fpath[BUFSIZ];
	strcpy(fpath,serverdir);
	strcat(fpath, path.c_str());
	int res = open(fpath, cdata.flags());
	if( res < 0) {
		res = -errno;
	}
	nfs::Result result;
	result.set_res(res);
	
	result.SerializeToString(&data);
	printf("open path: %s \n", fpath);
	send(client_sockfd, data.c_str(), data.length(), 0);
	close(res);
}

//3
void access_server(string path, int mask){
	string data;
	char fpath[1000];
	strcpy(fpath,serverdir);
	strcat(fpath, path.c_str());
	int res = access(fpath, mask);
	if( res < 0) {
		res = -errno;
	}
	nfs::Result result;
	result.set_res(res);
	result.SerializeToString(&data);
	printf("access res: %d \n", res);
	printf("access path: %s \n", fpath);
	send(client_sockfd, data.c_str(), data.length(), 0);
}

//4
void read_dir(string path){
	DIR *dp;
	struct dirent *de;
	string data;
	char fpath[1000];
	strcpy(fpath,serverdir);
	strcat(fpath, path.c_str());
	dp = opendir(fpath);
	
	printf("readdir res: %d: \n", res);
	printf("readdir path: %s \n", fpath);
	nfs::Filelist list;
	if (dp == NULL)
	{
		list.set_res(-1);
		list.SerializeToString(&data);
		send(client_sockfd,data.c_str(), data.length(), 0);
		return;
	}
	while ((de = readdir(dp)) != NULL) {
		list.add_filename(de->d_name);
	}
	closedir(dp);
	list.set_res(0);
	list.SerializeToString(&data);
	send(client_sockfd,data.c_str(), data.length(), 0);
}

//5
void read_link(string path, int size){
	string data;
	char buf[size+1];
	char fpath[1000];
	strcpy(fpath,serverdir);
	strcat(fpath, path.c_str());
	int res = readlink(fpath, buf, size);
	if( res < 0) {
		res = -errno;
	}

	printf("readlink res: %d: \n", res);

	nfs::Buffer buffer;
	buffer.set_buf(buf);
	buffer.set_res(res);
	buffer.SerializeToString(&data);
	send(client_sockfd, data.c_str(), data.length(), 0);
}

//6
void make_node(string path, int mode, int rdev, int flag){
	string data;
	int res;
	char fpath[1000];
	strcpy(fpath,serverdir);
	strcat(fpath, path.c_str());
	if(flag ==1){res =  open(fpath,  O_CREAT | O_EXCL | O_WRONLY, 33204);}
	else if(flag ==2){ res = mkfifo(fpath, 33204);}
	else{ res = mknod(fpath, 33204, rdev);}
	
	if( res < 0) {
		res = -errno;
	}
	printf("mknod res: %d: \n", res);
	nfs::Result result;
	result.set_res(res);
	result.SerializeToString(&data);

	send(client_sockfd, data.c_str(), data.length(), 0);
}

//7
void make_dir(string path, int mode){
	string data;
	char fpath[1000];
	strcpy(fpath,serverdir);
	strcat(fpath, path.c_str());
	int res = mkdir(fpath, 16893);
	if( res < 0) {
		res = -errno;
	}
	printf("mkdir res: %d: \n", res);
	nfs::Result result;
	result.set_res(res);
	result.SerializeToString(&data);

	send(client_sockfd, data.c_str(), data.length(), 0);
}
//8
void un_link(string path){
	string data;
	char fpath[1000];
	strcpy(fpath,serverdir);
	strcat(fpath, path.c_str());
	int res = unlink(fpath);
	if( res < 0) {
		res = -errno;
	}
	nfs::Result result;
	result.set_res(res);
	result.SerializeToString(&data);
	printf("unlink res: %d: \n", res);
	send(client_sockfd, data.c_str(), data.length(), 0);
}

//9
void rm_dir(string path){
	string data;
	char fpath[1000];
	strcpy(fpath,serverdir);
	strcat(fpath, path.c_str());
	int res = rmdir(fpath);
	if( res < 0) {
		res = -errno;
	}
	printf("rmdir res: %d: \n", res);
	nfs::Result result;
	result.set_res(res);
	result.SerializeToString(&data);

	send(client_sockfd, data.c_str(), data.length(), 0);
}

//10
void sym_link(string from, string to){
	string data;
	char frompath[1000];
	strcpy(frompath,serverdir);
	strcat(frompath, from.c_str());
	char topath[1000];
	strcpy(topath,serverdir);
	strcat(topath, to.c_str());
	int res = symlink(frompath, topath);
	if( res < 0) {
		res = -errno;
	}
	nfs::Result result;
	result.set_res(res);
	result.SerializeToString(&data);
	printf("symlink res: %d: \n", res);
	send(client_sockfd, data.c_str(), data.length(), 0);
}

//11
void re_name(string from, string to){
	string data;
	char frompath[1000];
	strcpy(frompath,serverdir);
	strcat(frompath, from.c_str());
	char topath[1000];
	strcpy(topath,serverdir);
	strcat(topath, to.c_str());
	int res = rename(frompath, topath);
	if( res < 0) {
		res = -errno;
	}
	nfs::Result result;
	result.set_res(res);
	result.SerializeToString(&data);
	printf("rename res: %d: \n", res);
	send(client_sockfd, data.c_str(), data.length(), 0);
}

//12
void link_server(string from, string to){
	string data;
	char frompath[1000];
	strcpy(frompath,serverdir);
	strcat(frompath, from.c_str());
	char topath[1000];
	strcpy(topath,serverdir);
	strcat(topath, to.c_str());
	int res = link(frompath, topath);
	if( res < 0) {
		res = -errno;
	}
	nfs::Result result;
	result.set_res(res);
	result.SerializeToString(&data);
	printf("link res: %d: \n", res);
	send(client_sockfd, data.c_str(), data.length(), 0);
}

//13
void ch_mod(string path, int mode){
	string data;
	char fpath[1000];
	strcpy(fpath,serverdir);
	strcat(fpath, path.c_str());
	int res = chmod(fpath, mode);
	if( res < 0) {
		res = -errno;
	}
	nfs::Result result;
	result.set_res(res);
	result.SerializeToString(&data);
	printf("chmod res: %d: \n", res);
	send(client_sockfd, data.c_str(), data.length(), 0);
}

//14
void ch_own(string path, int uid, int gid){
	string data;
	char fpath[1000];
	strcpy(fpath,serverdir);
	strcat(fpath, path.c_str());
	int res = chown(fpath, uid, gid);
	if( res < 0) {
		res = -errno;
	}
	nfs::Result result;
	result.set_res(res);
	result.SerializeToString(&data);
	printf("open res: %d: \n", res);
	send(client_sockfd, data.c_str(), data.length(), 0);
}

//15
void trun_cate(string path, nfs::CData cdata){
	string data;
	char fpath[1000];
	strcpy(fpath,serverdir);
	strcat(fpath, path.c_str());
	int res = truncate(fpath, cdata.size());
	if( res < 0) {
		res = -errno;
	}
	nfs::Result result;
	result.set_res(res);
	result.SerializeToString(&data);
	printf("truncate res: %d: \n", res);
	send(client_sockfd, data.c_str(), data.length(), 0);
}

//16
void read_server(string path, nfs::CData cdata){
	int fd, res;
	struct dirent *de;
	string data;
	char fpath[1000];
	char buf[BUFSIZ];
	strcpy(fpath,serverdir);
	strcat(fpath, path.c_str());

	fd = open(fpath, O_RDONLY);
	if(fd<0){
		res = -errno;
	}
	else{
		res = pread(fd, buf, cdata.size(), cdata.offset());
		if( res < 0) {
			res = -errno;
		}else res = 0;
	}
	printf("read buf: %s \n", buf);

	nfs::Buffer buffer;
	buffer.set_buf(buf);
	buffer.set_res(res);
	buffer.SerializeToString(&data);
	send(client_sockfd, data.c_str(), data.length(), 0);
	close(fd);
}

//17
void write_server(string path, nfs::CData cdata){
	int fd, res;
	string data;
	char fpath[1000];
	strcpy(fpath,serverdir);
	strcat(fpath, path.c_str());
	printf("write\n");
	fd = open(fpath, O_WRONLY);
	printf("write\n");
	if(fd<0){
		printf("write fail\n");
		res = -errno;
	}
	else{
		
		res = pwrite(fd, cdata.buf().c_str(), cdata.size(), cdata.offset());
		printf("write offset: %d: \n", res);
		if( res < 0) {
			res = -errno;
		}
	}
	nfs::Result result;
	result.set_res(res);
	result.SerializeToString(&data);
	printf("write res: %d: \n", res);
	send(client_sockfd, data.c_str(), data.length(), 0);
	close(fd);
}

int main(int argc, char **argv)
{
    

    char buf[1000];
    int f;
    if (argc != 2)
    {
        printf("Usage : ./zipcode [port]\n");
        printf("예  : ./zipcode 4444\n");
        exit(0);
    }

    state = 0;

    // internet 기반의 소켓 생성 (INET)
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error : ");
        exit(0);
    }
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(atoi(argv[1]));

    state = bind(server_sockfd , (struct sockaddr *)&serveraddr,
            sizeof(serveraddr));
    if (state == -1)
    {
        perror("bind error : ");
        exit(0);
    }

    state = listen(server_sockfd, 5);
    if (state == -1)
    {
        perror("listen error : ");
        exit(0);
    }


//////////////////////////////////////////////////////////////////////////////////
	     int pid; 
	     while (true)   
	     {

	         client_sockfd = accept(server_sockfd, (struct sockaddr *)&clientaddr, &client_len); 

	         if (client_sockfd < 0) 
	         {
	         	exit(0);
	         }
	         
	         pid = fork(); 

	         if (pid < 0)
	         {
	         	//fprintf(stderr,"\n\tERROR, couldn't generate process.");
	         	exit(0);
	         } 

	         if (pid == 0)  
	         {
	            close(server_sockfd); 
	            f= read_from_socket(client_sockfd);
	             exit(0);
		   if(f==0)return 0;;
	         }
	         else 
	         { 
				close(client_sockfd); 
			 }	

	     } 		


	     close(server_sockfd); 
	     return 0;
	 }
/////////////////////////////////////////////////////////////////
int read_from_socket(int  client_sockfd)
    {
	    char buf[BUFSIZ];
            memset(buf, '0', 1000);
            if (recv(client_sockfd, buf, BUFSIZ, 0) <= 0)
            {
                close(client_sockfd);
		return 1;
            }
   
            if (strncmp(buf, "quit",4) == 0)
            {
		 printf("quit\n");
                write(client_sockfd, "bye bye", 8);
                close(client_sockfd);
		return 1;
            }
	    if (strncmp(buf, "close",5) == 0)
            {
		 printf("close\n");
                write(client_sockfd, "bye bye", 8);
                close(client_sockfd);
                return 0;
            }
	    nfs::CData cdata;
	    cdata.ParseFromString(buf);
	   switch(cdata.call()){
		case 1:  get_attr(cdata.path());  			  break;
		case 2:  open_server(cdata.path(), cdata); 	  break;
		case 3:  access_server(cdata.path(), cdata.mask());	  break;
		case 4:  read_dir(cdata.path()); 			  break;
		case 5:  read_link(cdata.path(), cdata.size()); 	  break;
		case 6:  make_node(cdata.path(), cdata.mode(), cdata.rdev(), cdata.flags()); break;
		case 7:  make_dir(cdata.path(), cdata.mode());  	  break;
		case 8:  un_link(cdata.path());                           break;
		case 9:  rm_dir(cdata.path());                            break;
		case 10: sym_link(cdata.frompath(), cdata.topath());      break;
		case 11: re_name(cdata.frompath(), cdata.topath());       break;
		case 12: link_server(cdata.frompath(), cdata.topath());   break;
		case 13: ch_mod(cdata.path(), cdata.mode());              break;
		case 14: ch_own(cdata.path(), cdata.uid(), cdata.gid());  break;
		case 15: trun_cate(cdata.path(), cdata);	  	  break;
		case 16: read_server(cdata.path(), cdata); break;
		case 17: write_server(cdata.path(), cdata); break;
		case 18: close(server_sockfd); exit(0); break;
 	  	default:  break;
	   }
	   return 1;
	}









