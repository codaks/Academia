/*
Author: 	Chitransh Kulshrestha
Roll No.: 	MT2023021
Date: 		04/10/2023
*/

#include "../macros.h"
void intToString(char* buffer, int value);
void viewOfferingCourses(char* login_id,int sock);
void addNewCourse(int sock);
void removeCourse(int sock);
void updateCourse(int sock);
void changePassword(int sock);

int facultyMenu(char* login_id,int  sock){//used in client.c
	printf("\n------- Welcome to Faculty Menu --------\n");
	printf("1. View Offering Courses \n");
	printf("2. Add New Courses \n");
	printf("3. Remove Courses from Catalog\n");
	printf("4. Update Course Details \n");
	printf("5. Change Password \n");
	printf("6. Logout & Exit\n");
	
	int choice;
	char prompt[] = "Enter Your Choice: ";
	char buffer[10];

    write(STDOUT_FILENO, prompt, sizeof(prompt));

    ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (bytesRead == -1) {
        perror("read");
        return 0;
    }

    buffer[bytesRead] = '\0';

    if (sscanf(buffer, "%d", &choice) != 1) {
        fprintf(stderr, "\nInvalid input. Please enter an integer.\n");
        return 0;
    }
	write(sock, &choice, sizeof(choice));

	switch(choice) {
		case 1: viewOfferingCourses(login_id, sock);
		break;

		case 2: addNewCourse(sock);
		break;

		case 3: removeCourse(sock);
		break;

		case 4: updateCourse(sock);
		break;

		case 5: changePassword(sock);
		break;

		case 6: return -1;
	}
}

void viewOfferingCourses(char* login_id,int sock) {
	struct Courses course[6];
	int n;
	read(sock, &n, sizeof(n));
	read(sock, &course, sizeof(course));
	if(n >= 1) {
		for(int i = 0; i < n; i++) {
			write(STDOUT_FILENO, "******Course Details*****\n", strlen("******Course Details*****\n"));
    		write(STDOUT_FILENO, "Course Id: ", strlen("Course Id: "));
			write(STDOUT_FILENO, course[i].course_id, strlen(course[i].course_id));
			write(STDOUT_FILENO, "\nName: ", strlen("\nName: "));
			write(STDOUT_FILENO, course[i].name, strlen(course[i].name));
			write(STDOUT_FILENO, "\nDepartment: ", strlen("\nDepartment: "));
			write(STDOUT_FILENO, course[i].department, strlen(course[i].department));
			write(STDOUT_FILENO, "\nCredits: ", strlen("\nCredits: "));
			write(STDOUT_FILENO, &course[i].credits, sizeof(course[i].credits));
			write(STDOUT_FILENO, "\nNo. Of Available Seats: ", strlen("\nNo. Of Available Seats: "));
			write(STDOUT_FILENO, &course[i].no_of_available_seats, sizeof(course[i].no_of_available_seats));
			write(STDOUT_FILENO, "\nNo. Of Seats: ", strlen("\nNo. Of Seats: "));
			write(STDOUT_FILENO, &course[i].no_of_seats, sizeof(course[i].no_of_seats));
			write(STDOUT_FILENO, "\n \n", strlen("\n \n"));
		}	
	}
	else {
		write(STDOUT_FILENO, "No Course Found!", 17);
	}
}


void addNewCourse(int sock) {
	struct Courses course;

	int isCourseFull;

	read(sock, &isCourseFull, sizeof(isCourseFull));

	if(isCourseFull) {
		write(STDOUT_FILENO, "You Cannot Add Any More Courses \n", sizeof("You Cannot Add Any More Courses \n"));
		return;
	}

	write(STDOUT_FILENO, "Enter Course Name: ", 20);
    read(STDIN_FILENO, course.name, sizeof(course.name));

    write(STDOUT_FILENO, "Enter Department: ", 19);
    read(STDIN_FILENO, course.department, sizeof(course.department));

    write(STDOUT_FILENO, "Enter Number of Seats: ", 24);
    read(STDIN_FILENO, &course.no_of_seats, sizeof(course.no_of_seats));

    write(STDOUT_FILENO, "Enter Credits: ", 14);
    read(STDIN_FILENO, &course.credits, sizeof(course.credits));

    write(STDOUT_FILENO, "Enter Available Seats: ", 23);
    read(STDIN_FILENO, &course.no_of_available_seats, sizeof(course.no_of_available_seats));

	write(sock, &course, sizeof(course));
}


void removeCourse(int sock) {
	char courseId[5];
	int isExist;
	int isValid;
	write(STDOUT_FILENO, "Enter Course Id to delete: ", sizeof("Enter Course Id to delete: "));
	read(STDIN_FILENO, &courseId, sizeof(courseId));

	write(sock, &courseId, sizeof(courseId));
	read(sock, &isValid, sizeof(isValid));
	if(!isValid) {
		write(STDOUT_FILENO, "\nUnable to Remove Course at the momemt, please try after sometime...", sizeof("Unable to Remove Course at the momemt, please try after sometime..."));
		return;
	}
	read(sock, &isExist, sizeof(isExist));

	if(!isExist) {
		write(STDOUT_FILENO, "\nCourse with the given course id doesn't exist", sizeof("\nCourse with the given course id doesn't exist"));
	}
	else {
		write(STDOUT_FILENO, "\nCourse Deleted Successfully", sizeof("\nCourse Deleted Successfully"));
	}
	return;
}


void updateCourse(int sock) {
	char courseId[5];
	int isExist, isValid;
	char intBuffer[20];
	write(STDOUT_FILENO, "Enter Course Id for Update: ", sizeof("Enter Course Id for Update: "));
	read(STDIN_FILENO, &courseId, sizeof(courseId));

	write(sock, &courseId, sizeof(courseId));
	read(sock, &isValid, sizeof(isValid));
	if(!isValid) {
		write(STDOUT_FILENO, "\nUnable to Update Course at the momemt, please try after sometime...", sizeof("Unable to Remove Course at the momemt, please try after sometime..."));
		return;
	}
	read(sock, &isExist, sizeof(isExist));

	if(!isExist) {
		write(STDOUT_FILENO, "\nCourse with the given course id doesn't exist", sizeof("\nCourse with the given course id doesn't exist"));
	}

	struct Courses course;
	read(sock, &course, sizeof(struct Courses));
	printf("name %s\n", course.name);
	printf("dept %s\n", course.department);
	printf("noOfSeats %d\n", course.no_of_seats);

	char courseName[30];
	write(STDOUT_FILENO, "\nEnter Course Name (", sizeof("\nEnter Course Name ("));
	write(STDOUT_FILENO, course.name, strlen(course.name));
	write(STDOUT_FILENO, "): ", sizeof("): "));
	read(STDIN_FILENO, &courseName, sizeof(courseName));

	if(strlen(courseName) > 1) {
		strcpy(course.name, courseName);
	}

	char courseDepartment[20];
	write(STDOUT_FILENO, "\nEnter Department Name (", sizeof("\nEnter Department Name ("));
	write(STDOUT_FILENO, course.department, strlen(course.department)-1);
	write(STDOUT_FILENO, "): ", sizeof("): "));
	read(STDIN_FILENO, &courseDepartment, sizeof(courseDepartment));

	if(strlen(courseDepartment) > 1) {
		strcpy(course.department, courseDepartment);
	}

	int noOfSeats;
	write(STDOUT_FILENO, "Enter No. Of Seats (", sizeof("Enter No. Of Seats ("));
	sprintf(intBuffer, "%d", course.no_of_seats);
	write(STDOUT_FILENO, intBuffer, strlen(intBuffer));
	write(STDOUT_FILENO, "): ", sizeof("): "));
	read(STDIN_FILENO, &noOfSeats, sizeof(noOfSeats));

	if(noOfSeats >= 0) {
		course.no_of_seats = noOfSeats;
	}

	int noOfAvailableSeats;
	write(STDOUT_FILENO, "Enter No. Of Available Seats (", sizeof("Enter No. Of Available Seats ("));
	sprintf(intBuffer, "%d", course.no_of_available_seats);
	// intToString(intBuffer, course.no_of_available_seats);
	write(STDOUT_FILENO, intBuffer, strlen(intBuffer));
	write(STDOUT_FILENO, "): ", sizeof("): "));
	read(STDIN_FILENO, &noOfAvailableSeats, sizeof(noOfAvailableSeats));

	if(noOfAvailableSeats >= 0) {
		course.no_of_seats = noOfAvailableSeats;
	}

	int credits;
	write(STDOUT_FILENO, "Enter Credits (", sizeof("Enter Credits ("));
	sprintf(intBuffer, "%d", course.credits);
	// intToString(intBuffer, course.credits);
	write(STDOUT_FILENO, intBuffer, strlen(intBuffer));
	write(STDOUT_FILENO, "): ", sizeof("): "));
	read(STDIN_FILENO, &credits, sizeof(credits));

	if(credits >= 0) {
		course.credits = credits;
	}
	
	printf("Course name %s \n", course.name);
	write(sock, &course, sizeof(struct Courses));

	read(sock, &isValid, sizeof(isValid));
	if(!isValid) {
		write(STDOUT_FILENO, "\nUnable to Update Course at the moment, please try after sometime...", sizeof("Unable to Remove Course at the momemt, please try after sometime..."));
		return;
	}
}


void changePassword(int sock) {

}


void intToString(char* buffer, int value) {
    int i = 0;
    int isNegative = 0;

    if (value < 0) {
        isNegative = 1;
        value = -value;
    }

    do {
        buffer[i++] = value % 10 + '0';
        value /= 10;
    } while (value);

    if (isNegative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    // Reverse the string
    int length = strlen(buffer);
    for (int j = 0; j < length / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[length - j - 1];
        buffer[length - j - 1] = temp;
    }
}