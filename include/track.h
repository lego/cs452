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



void InitTrack();

void WhereAmI(int train);

path_t GetPath(int src, int dest);

void Navigate(int train, int dest);

void Move(int train, path_t *p, int speed);

int CalcTime(int train, path_t *p, int speed);

int AccelDist(int train, int speed);

int DeaccelDist(int train, int speed);

int Velocity(int train, int speed);

int AccelTime(int train, int speed);

int DeaccelTime(int train, int speed);

#define REVERSE_DIR 1
#define FORWARD_DIR 0
int GetDirection(int train, path_t *p);

int CalculateDistance(int velocity, int time);
