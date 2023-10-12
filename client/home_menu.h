/*
Author: 	Vicky Panchal
Roll No.: 	MT2023098
Date: 		04/10/2023
*/

#include "admin_menu.h"
#include "student_menu.h"
#include "faculty_menu.h"

#include "../macros.h"

int homeMenu(int opt,int  sock){//used in client.c
	if(opt==1 || opt == 2 || opt == 3){
		char login_id[10];
		char password[PASSWORD_LENGTH];
		printf("Login-Id: ");
		scanf("%s", login_id);
		strcpy(password,getpass("Enter the password: "));
		
		write(sock, &login_id, sizeof(login_id));
		write(sock, &password, sizeof(password));

		int valid_login;
		int role;
		read(sock, &valid_login, sizeof(valid_login));
		printf("valid login is: %d\n", valid_login);
		if(valid_login == 1){
			read(sock, &role, sizeof(role));
			switch(role) {
				case 1: while(adminMenu(role, sock)!=-1);
				break;

				case 2: while(studentMenu(role, sock)!=-1);
				break;

				case 3: while(facultyMenu(role, sock)!=-1);
				break;

				default: printf("Invalid Choice \n");
			}
			system("clear");
			return 1;
		}
		else{
			printf("Login Failed : Multiple Login for this user is not allowed.\n");
			while(getchar()!='\n');
			getchar();
			return 1;
		}
	}
	else
		return 3;
}