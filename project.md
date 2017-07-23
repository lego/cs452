## Train handling

- [ ] Handle collision case for routing (what to do, and with whom)
- [ ] Make routing account for reserved tracks to prevent simple avoidances
- [ ] Making sure a train keeps a segment reserved when it's not in motion
  - [ ] The train is moving (i.e. from attribution)
  - [ ] The train stops (part of attribution)
- [ ] Handle re-routing in the event of a failed switch
- [ ] Pathing that can start the train in a reversed direction
- [ ] Pathing which can have reversals on the route (i.e. kind of multi-destintation paths)
  - This can be done by changing dijkstras to go through `node.reverse` with X cost, then expanding the path whenever we cross a NODE_MERGE to the other branch. X is the cost to the next sensor + coming back (not account for slow down / speed up of reverse manuever)
- [ ] Possibly navigating to exits, only if we need it.

## Game

- [ ] Come up with a set of game mechanics and rules (e.g. stations where, what restrictions do paths have?)
- [ ] Being able to introduce a new train into the game
- [ ] Spawning of "people" at random stations
- [ ] Transportation of "people" via. a train
- [ ] Way to interface into the trains to control them
  - I suggest a set of functions which wrap messages to Executor
- [ ] Hooks into the train movement for doing our game conditions:
 - [ ] When the train arrives at a stations / node
