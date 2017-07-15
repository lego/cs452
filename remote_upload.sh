#!/bin/bash
# Assumes $1 is a file with no directors
# Linked to the PATH in the school environment
REMOTE_USER=j5pereir
scp $1 $REMOTE_USER@linux.student.cs.uwaterloo.ca:/u/cs452/tftp/ARM/$REMOTE_USER/$1
