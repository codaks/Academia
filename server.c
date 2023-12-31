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

char *Account[4] = {"./database/accounts/admin", "./database/accounts/student", "./database/accounts/faculty", "./database/courses"};

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
void viewFaculty(int sock);
void activateStudent(int sock);
void blockStudent(int sock);
void updateStudentDetails(int sock);
void updateFaultyDetails(int sock);
void addNewCourse(char login_id[], int sock);
void viewOfferingCourses(char login_id[], int sock);
void viewStudent(int sock);
void addFaculty(int sock);
void removeCourse(char login_id[], int sock);
void changePassword(int sock, char login_id[]);
void viewAllCourses(char login_id[], int sock);



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
		lseek(fd, (id-1)*sizeof(struct Student), SEEK_SET);
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
		case 4: viewFaculty(sock);
		break;
		case 5: activateStudent(sock);
		break;
		case 6: blockStudent(sock);
		break;
		case 7: updateStudentDetails(sock);
		break;
		case 8: updateFaultyDetails(sock);
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
	student.isActive = 1;
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
		lseek(fd, (count-1)*sizeof(struct Student), SEEK_SET);
		write(fd, &student, sizeof(student));
	}
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);
	printf("\n Student Login Id: %s \n", student.login_id);
}

int studentMenu(int sock, char login_id[]) {

	int choice;
	read(sock, &choice, sizeof(choice));

	switch(choice) {
		case 1: viewAllCourses(login_id,sock);
		
		break;

		case 6: return -1;

		default: return -1;
	}
	return 0;
}

int facultyMenu(int sock, char login_id[]) {
	int choice;

	read(sock, &choice, sizeof(choice));

	switch(choice) {
		case 1: viewOfferingCourses(login_id, sock);
		break;

		case 2: addNewCourse(login_id, sock);
		break;

		case 3: removeCourse(login_id, sock);
		break;

		case 5: changePassword(sock,login_id);
		// case 4: updateCourse(login_id, sock);
		// break;

		case 6: return -1;

		default: return -1;
	}
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

	if(count==1)
		write(fd, &faculy, sizeof(faculy));
	else{
		lseek(fd, (count-1)*sizeof(struct Faculty), SEEK_SET);
		write(fd, &faculy, sizeof(faculy));
	}
	lseek(fd, count*sizeof(struct Faculty), SEEK_SET);
	
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);
	printf("\n Faculty Login Id: %s \n", faculy.login_id);
}

void viewStudent(int sock){
	printf("Inside the View Student Function \n");
	char id[10];
	read(sock,&id,sizeof(id));
	printf("Requested for %s\n",id);


	int fd = open(Account[1], O_RDWR);
	struct Student student;
	int count = 0;
	int int_id;


	if (strlen(id) >= 4 && strncmp(id, "MT", 2) == 0) {
    		char* number_str = id + 2; // Skip the first 2 characters ("MT")
    		int_id = atoi(number_str); // Convert the remaining characters to an integer
			printf("We Get Number as: %d\n",int_id);
    	} else {
    		printf("Invalid login_id format\n");
    	}

	count = int_id-1;
	int no_of_rec,status = 1;
	int count_fd = open(no_of[0], O_RDWR);
	int count_size = read(count_fd, &no_of_rec, sizeof(no_of_rec));
	printf("Total No of Records: %d Entered Record %d\n",no_of_rec,count);
	if(count_size<=0 || count>=no_of_rec){
		status = 0;
		write(sock,&status,sizeof(status));
	}
	else{
		write(sock,&status,sizeof(status));
		lseek(fd, count*sizeof(struct Student), SEEK_SET);
		read(fd,&student,sizeof(student));
		write(sock,&student,sizeof(student));
	}
	// printf("ID : %s \n",student.login_id);
	// printf("Name :%s \n",student.name);
	// printf("Age : %d \n",student.age);
	// printf("Email : %s \n",student.email);
	// printf("Address: %s \n",student.address);

	// while(read(fd,&student,sizeof(student))){
	// 	count++;
	// 	lseek(fd, count*sizeof(struct Student), SEEK_SET);
	// 	printf("Roll No: %s\n",student.login_id);
	// }
	close(fd);
}

void viewFaculty(int sock){
	printf("Inside the View Faculty Function \n");
	char id[10];
	read(sock,&id,sizeof(id));
	printf("Requested for %s\n",id);


	int fd = open(Account[2], O_RDWR);
	struct Faculty faculty;
	int count = 0;
	int int_id;


	if(strlen(id) >= 4 && strncmp(id, "FT", 2) == 0) {
    		char* number_str = id + 2; // Skip the first 2 characters ("MT")
    		int_id = atoi(number_str); // Convert the remaining characters to an integer
			printf("We Get Number as: %d\n",int_id);
    }
	else {
    		printf("Invalid login_id format\n");
    }

	count = int_id-1;
	int no_of_rec,status = 1;
	int count_fd = open(no_of[1], O_RDWR);
	int count_size = read(count_fd, &no_of_rec, sizeof(no_of_rec));
	printf("Total No of Records: %d Entered Record %d\n",no_of_rec,count);
	if(count_size<=0 || count>=no_of_rec){
		printf("Invalid Faculty Details");
		status = 0;
		write(sock,&status,sizeof(status));
	}
	else{
		write(sock,&status,sizeof(status));
		lseek(fd, count*sizeof(struct Faculty), SEEK_SET);
		read(fd,&faculty,sizeof(faculty));
		write(sock,&faculty,sizeof(faculty));
	}
	// printf("ID : %s \n",student.login_id);
	// printf("Name :%s \n",student.name);
	// printf("Age : %d \n",student.age);
	// printf("Email : %s \n",student.email);
	// printf("Address: %s \n",student.address);
	count= 0;
	lseek(fd, count*sizeof(struct Faculty), SEEK_SET);
	while(read(fd,&faculty,sizeof(faculty))){
		count++;
		printf("Roll No: %s\n",faculty.login_id);
		lseek(fd, count*sizeof(struct Faculty), SEEK_SET);
	}
	close(fd);
}

void activateStudent(int sock){
	printf("Inside the activateStudent Function \n");
	char id[10];
	read(sock,&id,sizeof(id));
	printf("Requested for %s\n",id);

	int int_id;
	int status = 1;
	int format_status = 1;
	if(strlen(id) >= 4 && strncmp(id, "MT", 2) == 0) {
    		char* number_str = id + 2; // Skip the first 2 characters ("MT")
    		int_id = atoi(number_str); // Convert the remaining characters to an integer
			printf("We Get Number as: %d\n",int_id);
    }
	else {
    		format_status = 0;
    		printf("Invalid login_id format\n");
			write(sock,&format_status,sizeof(format_status));
			return;
    }

	write(sock,&format_status,sizeof(format_status));
	int count = int_id;

	int no_of_rec,count_flag = 1;
	int count_fd = open(no_of[0], O_RDWR);
	int count_size = read(count_fd, &no_of_rec, sizeof(no_of_rec));
	printf("Total No of Records: %d Entered Record %d\n",no_of_rec,count);
	if(count_size<=0 || count>no_of_rec){
		printf("Invalid Faculty Details");
		count_flag = 0;
		write(sock,&count_flag,sizeof(count_flag));
		return;
	}
	write(sock,&count_flag,sizeof(count_flag));
	


	int fd = open(Account[1], O_RDWR);
	struct flock lock;

	lock.l_start = (count-1)*sizeof(struct Student); 
	lock.l_len = sizeof(struct Student);
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();
	lock.l_type = F_WRLCK;
	fcntl(fd,F_SETLK, &lock);
	// write(fd, &faculy, sizeof(struct Faculty));

	
	struct Student student;
	lseek(fd, (count-1)*sizeof(struct Student), SEEK_SET);
	read(fd, &student, sizeof(student));
	student.isActive = 1;
	lseek(fd, (count-1)*sizeof(struct Student), SEEK_SET);
	write(fd, &student, sizeof(student));
	
	// lseek(fd, count*sizeof(struct Faculty), SEEK_SET);
	
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);
}

void blockStudent(int sock){
	printf("Inside the blockStudent Function \n");
	char id[10];
	read(sock,&id,sizeof(id));
	printf("Requested for %s\n",id);

	int int_id;
	int format_status = 1;
	if(strlen(id) >= 4 && strncmp(id, "MT", 2) == 0) {
    		char* number_str = id + 2; // Skip the first 2 characters ("MT")
    		int_id = atoi(number_str); // Convert the remaining characters to an integer
			printf("We Get Number as: %d\n",int_id);
    }
	else {
			format_status = 0;
    		printf("Invalid login_id format\n");
			write(sock,&format_status,sizeof(format_status));
			return;
    }
	write(sock,&format_status,sizeof(format_status));
	int count = int_id;


	int no_of_rec,count_flag = 1;
	int count_fd = open(no_of[0], O_RDWR);
	int count_size = read(count_fd, &no_of_rec, sizeof(no_of_rec));
	printf("Total No of Records: %d Entered Record %d\n",no_of_rec,count);
	if(count_size<=0 || count>no_of_rec){
		printf("Invalid Faculty Details");
		count_flag = 0;
		write(sock,&count_flag,sizeof(count_flag));
		return;
	}
	write(sock,&count_flag,sizeof(count_flag));


	int fd = open(Account[1], O_RDWR);
	struct flock lock;

	lock.l_start = (count-1)*sizeof(struct Student); 
	lock.l_len = sizeof(struct Student);
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();
	lock.l_type = F_WRLCK;
	fcntl(fd,F_SETLK, &lock);
	// write(fd, &faculy, sizeof(struct Faculty));

	
	struct Student student;
	lseek(fd, (count-1)*sizeof(struct Student), SEEK_SET);
	read(fd, &student, sizeof(student));
	printf("Student ID: %s\n",student.login_id);
	student.isActive = 0;
	lseek(fd, (count-1)*sizeof(struct Student), SEEK_SET);
	write(fd, &student, sizeof(student));
	
	// lseek(fd, count*sizeof(struct Faculty), SEEK_SET);
	
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);
}

void updateStudentDetails(int sock){
	printf("Inside the updateStudent Function \n");
	char id[10];
	read(sock,&id,sizeof(id));
	printf("Requested for %s\n",id);

	int int_id;

	if(strlen(id) >= 4 && strncmp(id, "MT", 2) == 0) {
    		char* number_str = id + 2; // Skip the first 2 characters ("MT")
    		int_id = atoi(number_str); // Convert the remaining characters to an integer
			printf("We Get Number as: %d\n",int_id);
    }
	else {
    		printf("Invalid login_id format\n");
    }

	int count = int_id;
	int fd = open(Account[1], O_RDWR);
	struct flock lock;

	lock.l_start = (count-1)*sizeof(struct Student); 
	lock.l_len = sizeof(struct Student);
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();
	lock.l_type = F_WRLCK;
	fcntl(fd,F_SETLK, &lock);


	struct Student student;
	lseek(fd, (count-1)*sizeof(struct Student), SEEK_SET);
	read(fd, &student, sizeof(student));
	printf("Student ID: %s\n",student.login_id);

	int choice;
	read(sock,&choice,sizeof(choice));
	switch (choice)
	{
	case 1:
		char name[30];
		read(sock,&name,sizeof(name));
		// printf("Changed Name to %s\n",name);
		strcpy(student.name, name);
		printf("Changed Name to %s\n",student.name);
		lseek(fd, (count-1)*sizeof(struct Student), SEEK_SET);
		write(fd,&student,sizeof(student));
		break;

	case 2:
		int age;
		read(sock,&age,sizeof(age));
		// printf("Changed Name to %s\n",name);
		student.age = age;
		printf("Changed Age to %d\n",student.age);
		lseek(fd, (count-1)*sizeof(struct Student), SEEK_SET);
		write(fd,&student,sizeof(student));
		break;
	
	case 3:
		char address[50];
		read(sock,&address,sizeof(name));
		// printf("Changed Name to %s\n",name);
		strcpy(student.address, address);
		printf("Changed Address to %s\n",student.address);
		lseek(fd, (count-1)*sizeof(struct Student), SEEK_SET);
		write(fd,&student,sizeof(student));
		break;
	case 4:
		char email[30];
		read(sock,&email,sizeof(email));
		// printf("Changed Name to %s\n",name);
		strcpy(student.email, email);
		printf("Changed email to %s\n",student.email);
		lseek(fd, (count-1)*sizeof(struct Student), SEEK_SET);
		write(fd,&student,sizeof(student));
		break;
	default:
		break;
	}


	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);
}

void updateFaultyDetails(int sock){
	printf("Inside the update Faculty Function \n");
	char id[10];
	read(sock,&id,sizeof(id));
	printf("Requested for %s\n",id);

	int int_id;

	if(strlen(id) >= 4 && strncmp(id, "FT", 2) == 0) {
    		char* number_str = id + 2; // Skip the first 2 characters ("MT")
    		int_id = atoi(number_str); // Convert the remaining characters to an integer
			printf("We Get Number as: %d\n",int_id);
    }
	else {
    		printf("Invalid login_id format\n");
    }

	int count = int_id;
	int fd = open(Account[2], O_RDWR);
	struct flock lock;

	lock.l_start = (count-1)*sizeof(struct Faculty); 
	lock.l_len = sizeof(struct Faculty);
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();
	lock.l_type = F_WRLCK;
	fcntl(fd,F_SETLK, &lock);


	struct Faculty faculty;
	lseek(fd, (count-1)*sizeof(struct Faculty), SEEK_SET);
	read(fd, &faculty, sizeof(faculty));
	printf("Faculty ID: %s\n",faculty.login_id);

	int choice;
	read(sock,&choice,sizeof(choice));
	switch (choice)
	{
	case 1:
		char name[30];
		read(sock,&name,sizeof(name));
		// printf("Changed Name to %s\n",name);
		strcpy(faculty.name, name);
		printf("Changed Name to %s\n",faculty.name);
		lseek(fd, (count-1)*sizeof(struct Student), SEEK_SET);
		write(fd,&faculty,sizeof(faculty));
		break;

	case 2:
		int age;
		read(sock,&age,sizeof(age));
		// printf("Changed Name to %s\n",name);
		faculty.age = age;
		printf("Changed Age to %d\n",faculty.age);
		lseek(fd, (count-1)*sizeof(struct Student), SEEK_SET);
		write(fd,&faculty,sizeof(faculty));
		break;
	
	case 3:
		char address[50];
		read(sock,&address,sizeof(name));
		// printf("Changed Name to %s\n",name);
		strcpy(faculty.address, address);
		printf("Changed Address to %s\n",faculty.address);
		lseek(fd, (count-1)*sizeof(struct Student), SEEK_SET);
		write(fd,&faculty,sizeof(faculty));
		break;
	case 4:
		char email[30];
		read(sock,&email,sizeof(email));
		// printf("Changed Name to %s\n",name);
		strcpy(faculty.email, email);
		printf("Changed email to %s\n",faculty.email);
		lseek(fd, (count-1)*sizeof(struct Faculty), SEEK_SET);
		write(fd,&faculty,sizeof(faculty));
		break;
	default:
		break;
	}


	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);
}

void viewOfferingCourses(char login_id[], int sock) {
	int count;

	int count_fd = open(no_of[2], O_RDONLY);
	lseek(count_fd, 0, SEEK_SET);
	read(count_fd, &count, sizeof(count));
	close(count_fd);
	
	struct Courses courseItem;
	struct Courses course[6];
	int fd = open(Account[3], O_RDONLY);
	int i = 0;
	int cnt = 0;
	lseek(fd, 0, SEEK_SET);
	while(read(fd, &courseItem, sizeof(courseItem))>0) {
		cnt++;
		lseek(fd, cnt*sizeof(courseItem), SEEK_SET);
		if(!strcmp(courseItem.faculty_id, login_id) && courseItem.isActive) {
			course[i] = courseItem;
			i++;
		}
		printf("Course Name: %s \n",course->name);
	}
	write(sock, &i, sizeof(i));
	write(sock, &course, sizeof(course));
}

void addNewCourse(char login_id[], int sock) {
	int isCourseFull = 0;
	struct flock facultyLock;
	char num_str[4];
	strcpy(num_str, 2+login_id);
	num_str[3] = '\0';
	int id = atoi(num_str);

	int faculty_fd = open(Account[2], O_RDWR);
	struct Faculty faculty;

	facultyLock.l_start = (id-1)*sizeof(struct Faculty);  //lock on course record
	facultyLock.l_len = sizeof(struct Faculty);
	facultyLock.l_whence = SEEK_SET;
	facultyLock.l_pid = getpid();
	facultyLock.l_type = F_WRLCK;
	fcntl(faculty_fd,F_SETLK, &facultyLock);

	lseek(faculty_fd, (id-1)*sizeof(struct Faculty), SEEK_SET);
	read(faculty_fd, &faculty, sizeof(faculty));

	
	if(faculty.courseCount == 6) {
		isCourseFull = 1;
		write(sock, &isCourseFull, sizeof(isCourseFull));

		facultyLock.l_type = F_UNLCK;
		fcntl(faculty_fd, F_SETLK, &facultyLock);
		close(faculty_fd);
		return;
	}

	faculty.courseCount += 1;
	lseek(faculty_fd, (id-1)*sizeof(struct Faculty), SEEK_SET);
	write(faculty_fd, &faculty, sizeof(faculty));

	write(sock, &isCourseFull, sizeof(isCourseFull));

	facultyLock.l_type = F_UNLCK;
	fcntl(faculty_fd, F_SETLK, &facultyLock);
	close(faculty_fd);

	struct Courses course;

	read(sock, &course, sizeof(course));
	strcpy(course.faculty_id, login_id);
	course.isActive = 1;

	int count;
	int count_fd = open(no_of[2], O_RDWR);
	struct flock count_lock;
	count_lock.l_start = 0;
	count_lock.l_len = 0;
	count_lock.l_whence = SEEK_SET;
	count_lock.l_pid = getpid();
	count_lock.l_type = F_WRLCK;
	fcntl(count_fd, F_SETLKW, &count_lock);
	lseek(count_fd, 0, SEEK_SET);

	int count_size = read(count_fd, &count, sizeof(count));
	if(count_size <= 0) count = 0;
	count++;
	printf("count = %d\n", count);
	lseek(count_fd, 0, SEEK_SET);
	write(count_fd, &count, sizeof(count));

	char num_str2[4];
    snprintf(num_str2, sizeof(num_str2), "%03d", count);
	strcpy(course.course_id, "CS");
	strcat(course.course_id, num_str2);

	int fd = open(Account[3], O_RDWR);
	struct flock lock;

	lock.l_start = (count-1)*sizeof(struct Courses);  //lock on course record
	lock.l_len = sizeof(struct Courses);
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();
	lock.l_type = F_WRLCK;
	fcntl(fd,F_SETLK, &lock);

	count_lock.l_type = F_UNLCK;
	fcntl(count_fd, F_SETLK, &count_lock);
	close(count_fd);

	lseek(fd, (count-1)*sizeof(struct Courses), SEEK_SET);
	write(fd, &course, sizeof(course));
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);

	printf("\n Course Id: %s \n", course.course_id);
}

void removeCourse(char login_id[], int sock) {
	char courseId[5];
	int isExist = 0;
	int invalid = 0, valid = 1;
	read(sock, &courseId, sizeof(courseId));

	char num_str[4];
	strcpy(num_str, 2+courseId);
	num_str[3] = '\0';
	int id = atoi(num_str);

	struct flock lock;
	struct Courses course;
	int fd = open(Account[3], O_RDWR);

	lock.l_start = (id-1)*sizeof(struct Courses);
	lock.l_len = sizeof(struct Courses);
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();
	lock.l_type = F_WRLCK;
	if(fcntl(fd,F_SETLK, &lock)==-1) {
		write(sock, &invalid, sizeof(invalid));
		close(fd);
		return;
	}
	write(sock, &valid, sizeof(valid));
	lseek(fd, (id-1)*sizeof(struct Courses), SEEK_SET);
	read(fd, &course, sizeof(struct Courses));

	if(strcmp(course.course_id, courseId) || strcmp(course.faculty_id, login_id)) {
		write(sock, &isExist, sizeof(isExist));
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		return;
	}
	isExist = 1;
	write(sock, &isExist, sizeof(isExist));
	course.isActive = 0;
	lseek(fd, (id-1)*sizeof(struct Courses), SEEK_SET);
	write(fd, &course, sizeof(struct Courses));
	
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);
}

void changePassword(int sock, char login_id[]) {
	char num_str[4];
	strcpy(num_str, 2+login_id);
	num_str[3] = '\0';
	int id = atoi(num_str);

	char password[PASSWORD_LENGTH];
	int passLen;
	// read(sock, &passLen, sizeof(passLen));
	read(sock, &password, sizeof(password));


	printf("password: %s\n", password);
	struct flock lock;

	if(login_id[0] == 'S') {
		int fd = open(Account[1], O_RDWR);
		struct Student student;
		lock.l_start = (id-1)*sizeof(struct Student);  //lock on student record
		lock.l_len = sizeof(struct Student);
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		lock.l_type = F_WRLCK;
		fcntl(fd,F_SETLK, &lock);

		lseek(fd, (id-1)*sizeof(struct Student), SEEK_SET);
		read(fd, &student, sizeof(struct Student));

		strcpy(student.password, password);

		lseek(fd, (id-1)*sizeof(struct Student), SEEK_SET);
		write(fd, &student, sizeof(struct Student));

		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);

		printf("updated password: %s\n", student.password);
	}
	else if(login_id[0] == 'F') {
		int fd = open(Account[2], O_RDWR);
		struct Faculty faculty;

		lock.l_start = (id-1)*sizeof(struct Faculty);  //lock on faculty record
		lock.l_len = sizeof(struct Faculty);
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();
		lock.l_type = F_WRLCK;
		fcntl(fd,F_SETLK, &lock);

		lseek(fd, (id-1)*sizeof(struct Faculty), SEEK_SET);
		read(fd, &faculty, sizeof(struct Faculty));

		strcpy(faculty.password, password);

		lseek(fd, (id-1)*sizeof(struct Faculty), SEEK_SET);
		write(fd, &faculty, sizeof(struct Faculty));

		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);

		printf("updated password: %s\n", faculty.password);
	}
}

void viewAllCourses(char login_id[], int sock) {
	int count;

	int count_fd = open(no_of[2], O_RDONLY);
	lseek(count_fd, 0, SEEK_SET);
	read(count_fd, &count, sizeof(count));
	close(count_fd);
	
	struct Courses courseItem;
	struct Courses course[count];
	int fd = open(Account[3], O_RDONLY);
	int i = 0;
	int cnt = 0;
	lseek(fd, 0, SEEK_SET);
	while(read(fd, &courseItem, sizeof(courseItem))>0) {
		cnt++;
		lseek(fd, cnt*sizeof(courseItem), SEEK_SET);
		if(courseItem.isActive) {
			course[i] = courseItem;
			i++;
		}
		printf("Course Name: %s \n",course->name);
	}
	write(sock, &i, sizeof(i));
	write(sock, &course, sizeof(course));
}


