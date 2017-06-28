#pragma once

#include <stdbool.h>
#include <trains/track_node.h>
#include <trains/track_data.h>

#define TRAINS_MAX 80

extern track_node track[TRACK_MAX];

typedef struct Path {
  int dist; /* in millimetres */
  int len;
  track_node *src;
  track_node *dest;
  track_node *nodes[TRAINS_MAX];
} path_t;

// Unit macros for our fixed point calculations
#define CENTIMETRES(amt) MILLIMETRES(amt * 10)
#define MILLIMETRES(amt) amt
#define SECONDS(amt) MILLISECONDS(amt * 1000)
#define MILLISECONDS(amt) amt

void set_location(int train, int location);

int StoppingDistance(int train, int speed);

// Initializes the track and pre-computed routing information
void InitNavigation();

// Gets where the current train is
// NOTE: assumes the train is stationary
int WhereAmI(int train);

// Generates a path from A to B
void GetPath(path_t *p, int src, int dest);

void PrintPath(path_t *p);

// Navigates a train from A to B
void Navigate(int train, int speed, int src, int dest, bool include_stop);

// Get the accelerating distance
int AccelDist(int train, int speed);

// Get the deaccelerating distance (stopping distance)
int DeaccelDist(int train, int speed);

// Get the accelerating time
int AccelTime(int train, int speed);

// Get the deaccerating time, or stopping time
int DeaccelTime(int train, int speed);

#define REVERSE_DIR 1
#define FORWARD_DIR 0
// Get the direction orientation for a path compared to
// the trains current direction
int GetDirection(int train, path_t *p);

// Calculate a distance given a constant velocity and time
int CalculateDistance(int velocity, int time);

// Calculate the time given a distance and velocity
int CalculateTime(int distance, int velocity);

int Name2Node(char *name);

// Get the top / avg velocity for a train at a speed level
int Velocity(int train, int speed);

void dijkstra(int src, int dest);

int get_path(int src, int dest, track_node **path, int path_buf_size);

void set_velocity(int train, int speed, int velocity);

void set_stopping_distance(int train, int speed, int distance);


/*
  High level of how nagivating functions work:
  Navigate(src, dest):
   path = GetPath(src, dest)
   time = FigureOutTimeToNavigate(train, speed, path)
   direction = GetDirection(train, path)
   if direction == REVERSE:
     ReverseTrain(train)
   SetTrainSpeed(train, speed)
   return time
*/
