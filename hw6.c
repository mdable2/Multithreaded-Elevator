/*
HINTS:
1. Eventually no loops at all in passenger_request and elevator_ready
2. Initialize all barriers to 2
3. barrier wait(&barrier), 

*/


#include "hw6.h"
#include <stdio.h>
#include <pthread.h>

int passenger_count = 0;

struct Passenger {
    pthread_barrier_t barr;    
    enum {WAITING, ENTERED, EXITED} state;
    int from_floor;
    int to_floor;
    int use_elevator;
    int id;
    pthread_mutex_t lock;
} passengers[PASSENGERS];

struct Elevator {
    int id;
    int in_elevator;
    int at_floor;
    int floor;
    int open;
    int passengers;
    int trips;
    int current_floor;
    int direction;
    int occupancy;
    pthread_mutex_t lock;	
    struct Passenger *p;
    enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;	
} elevators[ELEVATORS];

/* Initialize struct, initalize mutex, initalize barrier */
void scheduler_init() {
    int i;

    /* Initialize all elevators	*/
    for (i = 0; i < ELEVATORS; i++) {
        pthread_mutex_init(&elevators[i].lock, NULL);
        elevators[i].current_floor=0;
        elevators[i].direction=-1;
        elevators[i].occupancy=0;
        elevators[i].state=ELEVATOR_CLOSED;
        elevators[i].p = NULL;
    }

    /* Initialize all passengers */
    for (i = 0; i < PASSENGERS; i++) {
        pthread_barrier_init(&passengers[i].barr, NULL, 2);
    }
}

// ahdhshshh
void passenger_request(int passenger, int from_floor, int to_floor, 
        void (*enter)(int, int), 
        void(*exit)(int, int))
{	
    //log(3, "PASSENGER REQUEST FOR %d\n", passenger);
    passengers[passenger].use_elevator = passenger % ELEVATORS;
    // elevators[passengers[passenger].use_elevator].p = &passengers[passenger];
    pthread_mutex_lock(&elevators[passengers[passenger].use_elevator].lock);

    //log(3, "INSIDE PASSENGER REQUEST LOCK\n", NULL);
    int index = passengers[passenger].use_elevator;
    passengers[passenger].id = passenger;
    int pass_id = passengers[passenger].id;
    passengers[passenger].from_floor = from_floor;
    passengers[passenger].to_floor = to_floor;
    elevators[index].p = &passengers[passenger];
    //log(3, "Assigned p for %d!\n", passenger);
    passengers[passenger].state = WAITING;
    passenger_count++;
    //log(3, "\n\n\n\nPassenger COUNT: %d\n\n\n\n", passenger_count);
    // if (passenger_count == 50) {
    // }

    pthread_mutex_unlock(&elevators[passengers[passenger].use_elevator].lock);
    pthread_mutex_lock(&elevators[passengers[passenger].use_elevator].lock);

    // wait for the elevator to arrive at our origin floor, then get in
    pthread_barrier_wait(&passengers[pass_id].barr);
    enter(pass_id, passengers[pass_id].use_elevator);
    elevators[index].occupancy++;
    passengers[passenger].state = ENTERED;
    pthread_barrier_wait(&passengers[pass_id].barr);
   
    pthread_mutex_unlock(&elevators[index].lock);

    // wait for the elevator at our destination floor, then get out
    pthread_mutex_lock(&elevators[index].lock);

    pthread_barrier_wait(&passengers[pass_id].barr);
    exit(pass_id, passengers[pass_id].use_elevator);
    passengers[passenger].state = EXITED;
    elevators[index].state = ELEVATOR_ARRIVED;
    elevators[index].occupancy--;
    elevators[index].p = NULL;

    pthread_mutex_unlock(&elevators[index].lock);
    pthread_barrier_wait(&passengers[pass_id].barr);    
}

void elevator_ready(int elevator, int at_floor, 
        void(*move_direction)(int, int), 
        void(*door_open)(int), void(*door_close)(int))
{

    //log(3, "The ELEVATOR: %d\n", elevator);
    //log(3, "The ELEVATOR STATE: %d\n", elevators[elevator].state);

    /* Check if at correct floor, if not, go to correct floor */
    if (elevators[elevator].state == ELEVATOR_CLOSED) {
        int difference;
        if (elevators[elevator].occupancy == 1) {
            //log(3, "In occupancy == 1.\n", NULL);

            difference = (elevators[elevator].p->to_floor - elevators[elevator].current_floor);
            elevators[elevator].current_floor = elevators[elevator].p->to_floor;
        }
        else {
            //log(3, "In occupancy == 0.\n", NULL);
            //log(3, "before calculated Difference VALUE: %d\n", difference);
            if(elevators[elevator].p == NULL) {
                //log(3, "in if null statement\n", NULL);
                return;
                //log(3, "From FLOOR of p: %d\n", elevators[elevator].p->from_floor);
                //log(3, "Null POINTER?: %d\n", &elevators[elevator].p);
            }


            difference = (elevators[elevator].p->from_floor - elevators[elevator].current_floor);
            //log(3, "Difference VALUE: %d\n", difference);
            elevators[elevator].current_floor = elevators[elevator].p->from_floor;
        }
        //log(3, "End of IF.\n", NULL);

        move_direction(elevator, difference);
        elevators[elevator].state = ELEVATOR_ARRIVED;
    }

    if (elevators[elevator].state == ELEVATOR_OPEN) {
        pthread_barrier_wait(&elevators[elevator].p->barr);

        door_close(elevator);
        elevators[elevator].state=ELEVATOR_CLOSED;
    }

    if (elevators[elevator].state == ELEVATOR_ARRIVED) {
        door_open(elevator);
        elevators[elevator].state = ELEVATOR_OPEN;
        pthread_barrier_wait(&elevators[elevator].p->barr);
    }



    // if ((elevators[elevator].current_floor != elevators[elevator].p->from_floor || elevators[elevator].current_floor != elevators[elevator].p->to_floor) 
    //         && elevators[elevator].p->state == ) {
    //     /* If occupied, drop passenger off */
    //     if (elevators[elevator].occupancy == 1) {
    //         elevators[elevator].current_floor == elevators[elevator].p->to_floor;
    //         door_open(elevator);
    //         elevators[elevator].state = ELEVATOR_OPEN;
    //         pthread_barrier_wait(&elevators[elevator].p->barr);
    //     }

    //     /* If empty, go get passenger */
    //     else {
    //         elevators[elevator].current_floor == elevators[elevator].p->from_floor;
    //         door_open(elevator);
    //         elevators[elevator].state = ELEVATOR_OPEN;
    //         pthread_barrier_wait(&elevators[elevator].p->barr);
    //     }
    // }

    // if (elevators[elevator].state == ELEVATOR_OPEN) {
    //     pthread_barrier_wait(&elevators[elevator].p->barr);
    //     door_close(elevator);
    //     elevators[elevator].state=ELEVATOR_CLOSED;
    //     pthread_barrier_wait(&elevators[elevator].p->barr);
    // }
    // else {
    //     // if(at_floor==0 || at_floor==FLOORS-1) 
    //     //     elevators[elevator].direction*=-1;
    //     // move_direction(elevator,elevators[elevator].direction);
    //     // elevators[elevator].current_floor=at_floor+elevators[elevator].direction;

    //     // if (elevators[elevator].current_floor == elevators[elevator].p->from_floor) {
    //     //     elevators[elevator].state=ELEVATOR_ARRIVED;
    //     // }
    // }
}
