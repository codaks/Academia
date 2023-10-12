/*
Author: 	Chitransh Kulshrestha	
Roll No.: 	MT2023021
Date: 		04/10/2023
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include "database/database.h"
#include "macros.h"

char *Account[3] = {"./database/accounts/admin", "./database/accounts/student", "./database/accounts/faculty"};

char *no_of[3] = {"./database/accounts/no_of_students", "./database/accounts/no_of_faculties", "./database/accounts/no_of_courses"};

void server_handler(int sig);
void service_client(int sock);
int login(int sock, int role);
int adminMenu(int sock, char login_id[]);
int studentMenu(int sock, char login_id[]);
int facultyMenu(int sock, char login_id[]);
void addStudent(int sock);
void addFaculty(int sock);
void viewStudent(int sock);


int main() {
    signal(SIGTSTP, server_handler);
	signal(SIGINT, server_handler);
	signal(SIGQUIT, server_handler);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd==-1) {
		printf("socket creation failed\n");
		exit(0);
	}
    int optval = 1;
	int optlen = sizeof(optval);
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen)==-1){
		/*The setsockopt() function provides an application program with the means to control socket behavior. 
		An application program can use setsockopt() to allocate buffer space, control timeouts, or permit socket data
		 broadcasts.*/
		printf("set socket options failed\n");
		exit(0);
	}
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	//The htonl() function converts the unsigned integer hostlong from host byte order to network byte order.
	sa.sin_port = htons(PORT);
	//The htons() function converts the unsigned short integer hostshort from host byte order to network byte order.

	if(bind(sockfd, (struct sockaddr *)&sa, sizeof(sa))==-1){
	/*
	 int bind(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
	 When a socket is created with socket(2), it exists in a name
       space (address family) but has no address assigned to it.  bind()
       assigns the address specified by addr to the socket referred to
       by the file descriptor sockfd.  addrlen specifies the size, in
       bytes, of the address structure pointed to by addr.
       Traditionally, this operation is called “assigning a name to a
       socket”.
	*/

		printf("binding port failed\n");
		exit(0);
	}
	if(listen(sockfd, 100)==-1){
		/*
		listen() marks the socket referred to by sockfd as a passive
        socket, that is, as a socket that will be used to accept incoming
        connection requests using accept(2).
		*/
		printf("listen failed\n");
		exit(0);
	}
	while(1){ 
		int connectedfd;
		if((connectedfd = accept(sockfd, (struct sockaddr *)NULL, NULL))==-1){
			/*
			The accept() system call is used with connection-based socket
       		types (SOCK_STREAM, SOCK_SEQPACKET).  It extracts the first
       		connection request on the queue of pending connections for the
       		listening socket, sockfd, creates a new connected socket, and
       		returns a new file descriptor referring to that socket
			*/
			printf("connection error\n");
			exit(0);
		}
		//in else part below, connectedfd has been assigned a value
		printf("Server Started Successfully....");
		pthread_t cli;
		if(fork()==0)//means child process will cater the client
			service_client(connectedfd);
	}

	close(sockfd);
	return 0;
}

void service_client(int sock){
	int func_id;
    printf("--------------------");
	read(sock, &func_id, sizeof(int));//value of func_id will be given by client. read() is being done on sock
	printf("Client [%d] connected\n", sock);
	while(1){		
		if(func_id >=1 && func_id <= 3) {
			login(sock, func_id);
			read(sock, &func_id, sizeof(int));
		}
		if(func_id > 3 || func_id < 1) break;
	}
	close(sock);
	printf("Client [%d] disconnected\n", sock);
}

void server_handler(int sig) {
	char str[5];
	printf("Do you want to stop the server(y/n)?\n");
	scanf("%s", str);
	if(strcmp("y", str) == 0) {
		exit(0);
	}
	return;
}

int login(int sock, int role){
	int fd, valid=1, invalid=0, login_success=0;
	char password[PASSWORD_LENGTH];
	char login_id[10];
	
	struct Admin admin;
	struct Student student;
	struct Faculty faculty;

	read(sock, &login_id, sizeof(login_id));
	read(sock, &password, sizeof(password));

	if((fd = open(Account[role-1], O_RDWR))==-1)printf("File Error\n");
	struct flock lock;
	char *idPrefix;
	if(role == 1) {
		idPrefix = "AD";
	} else if(role == 2) {
		idPrefix = "MT";
	} else {
		idPrefix = "FT";
	}
	int id;
	if (strlen(login_id) >= 4 && strncmp(login_id, idPrefix, 2) == 0) {
    	char* number_str = login_id + 2; // Skip the first 2 characters ("MT")
    	id = atoi(number_str); // Convert the remaining characters to an integer
    } else {
    	printf("Invalid login_id format\n");
    }
	printf("Login id = %d \n", id);
	if(role == 1){
		// admin
		printf("inside admin \n");
		lock.l_start = (id-1)*sizeof(struct Admin);  //lock on admin record
		lock.l_len = sizeof(struct Admin);
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		lock.l_type = F_WRLCK;
		if(fcntl(fd,F_SETLK, &lock) == -1) {
			valid = 0;
			write(sock, &valid, sizeof(valid));
		};
		lseek(fd, (id - 1)*sizeof(struct Admin), SEEK_SET);
		read(fd, &admin, sizeof(struct Admin));
		printf("admin login id: %sl\n", admin.login_id);
		printf("login id %sl\n", login_id);
		if(!strcmp(admin.login_id, login_id)) {
			printf("inside lock \n");
			printf("Actuall: %s \nEntered Value: %s",admin.password,password);
			if(!strcmp(admin.password, password)) {
				printf("inside password \n");
				write(sock, &valid, sizeof(valid));
				write(sock, &role, sizeof(role));
				while(-1!=adminMenu(sock, admin.login_id));
				login_success = 1;
			}
		}
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		if(login_success)
		return 3;
	}
	else if(role == 2){
		//student
		lock.l_start = (id-1)*sizeof(struct Student);  //lock on admin record
		lock.l_len = sizeof(struct Student);
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		lock.l_type = F_WRLCK;
		if(fcntl(fd,F_SETLK, &lock) == -1) {
			valid = 0;
			write(sock, &valid, sizeof(valid));
		};
		lseek(fd, (id)*sizeof(struct Student), SEEK_SET);
		read(fd, &student, sizeof(struct Student));
		printf("Student login %s\n", student.login_id);
		printf("login id %s\n", login_id);
		if(!strcmp(student.login_id, login_id)) {
			printf("s pwd %s\n", student.password);
			printf("pwd : %s\n", password);
			if(!strcmp(student.password, password)) {
				printf("inside if if\n");
				write(sock, &valid, sizeof(valid));
				write(sock, &role, sizeof(role));
				while(-1!=studentMenu(sock, student.login_id));
				login_success = 1;
			} else {
				printf("Invalid login");
			}
		}
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		if(login_success)
		return 3;
		
	}
	else if(role == 3){
		// Faculty
		lock.l_start = (id-1)*sizeof(struct Faculty);  //lock on admin record
		lock.l_len = sizeof(struct Faculty);
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		lock.l_type = F_WRLCK;
		fcntl(fd,F_SETLK, &lock);
		lseek(fd, (id - 1)*sizeof(struct Faculty), SEEK_CUR);
		read(fd, &faculty, sizeof(struct Faculty));
		if(!strcmp(faculty.login_id, login_id)) {
			if(!strcmp(faculty.password, password)) {
				write(sock, &valid, sizeof(valid));
				write(sock, &role, sizeof(role));
				while(-1!=facultyMenu(sock, faculty.login_id));
				login_success = 1;
			}
		}
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		if(login_success)
		return 3;
	}
	write(sock, &invalid, sizeof(invalid));
	return 3;
}

int adminMenu(int sock, char login_id[]) {
	int choice;
	read(sock, &choice, sizeof(choice));

	switch(choice) {
		case 1: addStudent(sock);
		break;
		case 2: viewStudent(sock);
		break;
		case 3: addFaculty(sock);
		break;
		case 9: return -1;
	}
	return 0;
}

void addStudent(int sock) {
	struct Student student;
	read(sock, &student, sizeof(struct Student));
	int count;
	int count_fd = open(no_of[0], O_RDWR);
	struct flock count_lock;
	count_lock.l_start = 0;
	count_lock.l_len = 0;
	count_lock.l_whence = SEEK_SET;
	count_lock.l_pid = getpid();
	count_lock.l_type = F_WRLCK;
	fcntl(count_fd, F_SETLK, &count_lock);
	lseek(count_fd, 0, SEEK_SET);
	int count_size = read(count_fd, &count, sizeof(count));
	printf("count size = %d\n", count_size);
	if(count_size <= 0) count = 0;
	count++;
	printf("count = %d\n", count);
	lseek(count_fd, 0, SEEK_SET);
	write(count_fd, &count, sizeof(count));
	count_lock.l_type = F_UNLCK;
	fcntl(count_fd, F_SETLK, &count_lock);
	close(count_fd);

	char num_str[4];
    snprintf(num_str, sizeof(num_str), "%03d", count);
	strcpy(student.login_id, "MT");
	strcat(student.login_id, num_str);
	int fd = open(Account[1], O_RDWR);
	struct flock lock;

	lock.l_start = (count-1)*sizeof(struct Student);  //lock on admin record
	lock.l_len = sizeof(struct Student);
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();
	lock.l_type = F_WRLCK;
	fcntl(fd,F_SETLK, &lock);
	// write(fd, &student, sizeof(struct Student));
	if(count==1)
		write(fd, &student, sizeof(student));
	else{
		lseek(fd, count*sizeof(struct Student), SEEK_SET);
		write(fd, &student, sizeof(student));
	}
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);
	printf("\n Student Login Id: %s \n", student.login_id);
}
int studentMenu(int sock, char login_id[]) {

	
	return 0;
}
int facultyMenu(int sock, char login_id[]) {
	return 0;
}


void addFaculty(int sock){

	struct Faculty faculy;
	read(sock, &faculy, sizeof(struct Faculty));
	int count;
	int count_fd = open(no_of[1], O_RDWR);
	struct flock count_lock;
	count_lock.l_start = 0;
	count_lock.l_len = 0;
	count_lock.l_whence = SEEK_SET;
	count_lock.l_pid = getpid();
	count_lock.l_type = F_WRLCK;
	fcntl(count_fd, F_SETLK, &count_lock);
	lseek(count_fd, 0, SEEK_SET);
	int count_size = read(count_fd, &count, sizeof(count));
	printf("count size = %d\n", count_size);
	if(count_size <= 0) count = 0;
	count++;
	printf("count = %d\n", count);
	lseek(count_fd, 0, SEEK_SET);
	write(count_fd, &count, sizeof(count));
	count_lock.l_type = F_UNLCK;
	fcntl(count_fd, F_SETLK, &count_lock);
	close(count_fd);

	char num_str[4];
    snprintf(num_str, sizeof(num_str), "%03d", count);
	strcpy(faculy.login_id, "FT");
	strcat(faculy.login_id, num_str);

	int fd = open(Account[2], O_RDWR);
	struct flock lock;

	lock.l_start = (count-1)*sizeof(struct Faculty);  //lock on admin record
	lock.l_len = sizeof(struct Faculty);
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();
	lock.l_type = F_WRLCK;
	fcntl(fd,F_SETLK, &lock);
	// write(fd, &faculy, sizeof(struct Faculty));
	lseek(fd, count*sizeof(struct Faculty), SEEK_SET);
	write(fd, &faculy, sizeof(faculy));
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);
	printf("\n Faculty Login Id: %s \n", faculy.login_id);
}

void viewStudent(int sock){
	printf("Inside the View Student Function \n");
	
	int fd = open(Account[1], O_RDWR);
	struct Student student;
	int count = 0;
	while(read(fd,&student,sizeof(student))){
		count++;
		lseek(fd, count*sizeof(struct Student), SEEK_SET);
		printf("Roll No: %s\n",student.login_id);
		
	}

}