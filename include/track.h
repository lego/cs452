#pragma once

typedef struct Path {
  int dist; /* in millimetres */
  int edge_count;
  track_node *src;
  track_node *dest;
  track_node nodes[TRAINS_MAX];
  track_edge edges[TRAINS_MAX];
} path_t;

// Unit macros for our fixed point calculations
#define CENTIMETRES(amt) MILLIMETRES(amt * 10)
#define MILLIMETRES(amt) amt
#define SECONDS(amt) MILLISECONDS(amt * 1000)
#define MILLISECONDS(amt) amt


// Initializes the track and pre-computed routing information
void InitTrack();

// Gets where the current train is
// NOTE: assumes the train is stationary
void WhereAmI(int train);

// Generates a path from A to B
path_t GetPath(int src, int dest);

// Navigates a train from A to B
void Navigate(int train, int dest);

// Moves a train from A to B. Does not support
// reversing for turn-outs, but ensures correct turn-out
// configuration
void Move(int train, path_t *p, int speed);

// Calculates the amount of time to get from A to B
// at a speed, including accelerating and deaccelerating
// NOTE: does not account for turn-out reversing, similar to Move
int CalcTime(int train, path_t *p, int speed);

// Get the accelerating distance
int AccelDist(int train, int speed);

// Get the deaccelerating distance (stopping distance)
int DeaccelDist(int train, int speed);

// Get the top / average velocity
int Velocity(int train, int speed);

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
