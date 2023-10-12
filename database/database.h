#include "../macros.h"

struct Courses {
    int course_id;
    char name[30];
    char department[20];
    int no_of_seats;
    int credits;
    int no_of_available_seats;
	int isActive;
};

struct Student {
    char login_id[10];
    char name[30];
    int age;
    char address[50];
    char password[PASSWORD_LENGTH];
    char email[30];
    int courses_enrolled[6];
	int isActive;
};

struct Faculty {
    char login_id[10];
    char name[30];
    int age;
    char address[30];
    char email[30];
    char password[PASSWORD_LENGTH];
    char courses_offered[6];
};

struct Admin {
    char login_id[10];
    char name[30];
    char password[PASSWORD_LENGTH];
    char email[30];
};