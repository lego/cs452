#pragma once

#include <basic.h>
#include <track/pathing.h>

#define VELOCITY_SAMPLES_MAX 5

// Unit macros for our fixed point calculations
#define CENTIMETRES(amt) MILLIMETRES(amt * 10)
#define MILLIMETRES(amt) amt
#define SECONDS(amt) MILLISECONDS(amt * 1000)
#define MILLISECONDS(amt) amt

// Initializes calibration information
void InitNavigation();

void set_location(int train, int location);

void SetPathSwitches(path_t *path);

int StoppingDistance(int train, int speed);

// Gets where the current train is
// NOTE: assumes the train is stationary
int WhereAmI(int train);

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

// Get the top / avg velocity for a train at a speed level
int Velocity(int train, int speed);

void set_velocity(int train, int speed, int velocity);
void record_velocity_sample(int train, int speed, int sample);

void set_stopping_distance(int train, int speed, int distance);
void offset_stopping_distance(int train, int speed, int offset);

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
