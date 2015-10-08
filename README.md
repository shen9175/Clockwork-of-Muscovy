# Clockwork-of-Muscovy
A home brew RPG game

The whole demo and introduction video is on youtube: https://youtu.be/8XbZs4j8Pi8

The game is totally build from zero, all the things and effects I 
have to implement myself including timer, animation, FPS calculation,
text display. Only use Microsoft old school GDI BitBlt, StrechBlt, 
AlphaBlend for graphic and sprite display. Use Microsoft Xaudio2
for the 3D positional Sound. All the others are implement myself.

This game is firstly built 2 years ago in the game class as
final project. But due to time contraint, at that time, the game
was full of bugs. Control is very  inaccurate and blunt.
And 4/5 changes will crash or stuck at some point with infinite loop.

At that time we have built basic ASCII art of game internal mouse control, path finding
and map switch mechanism. But we have only 2-3 days to implement with 2D graphic.
But we want add something more. we want to remake large campus map but manually
type in the collision data is too time consuming and we only have two days before
deadline. So we  chose to use large tile size to reduce the collision data input time.
But it came out that it was the wrost decision ever. Make the control very inaccurate and blunt.

After 2 years, I finally got some time to refine this project.
This time I spent more than 2 weeks try to make the control silky smooth,  bug
free and accurate under origianl 120x120 pixel tile but with pixel level accuracy.

The highlight of this refinement project is pixel level contrl accuracy under large 120x120 tile
Under 120 x 120 large tile, If you want easy programming,
 then give up control accuracy. If you want both of them,
then you have to do a lot of math. To get 120x120 pixel large tile
with pixel level accurate control, I developed "normal moving", 
"two-step moving" and "final pixel delivery" 3 types of moving 
and algorithms. The two step moving costs me a whole
morning with sketch paper deriving 6 linear geomety functions
constraints and setup up 12 types of situations to handle.

The control part debug make things even worse,
The half time is to solve the tons of different bugs.
Even "holding mouse move" such normal operation gives me a
lot of bugs. Not mentioned the 4 players control after adding
dynamic collsion in each characters and NPCs when they are
walking. Most of time they are stuck each other and dead locking
there. The good thing is after suffer these, now the game is bug free
with silky smooth control like Blizzard games.

This time I nearly rewrote the whole code, and try to make it very generic and
encapsulate module in the class and derived class. Make the code has more reuseability.

The walking part is encapsulate in a WalkingEngine used by AI controled NPCs
and a derived version of advanced WalkingEngine(Explorer) used by mouse controlled players.
And every character class has an instance of this WalkingEngine(or Advance WalkingEngie) like
the robot has installed the programmed legs.

The mouse control is implement as single click walk, holding without moving mouse walk, holding
with moving mouse walk, and shift click walk (player can walk all pre-defined the route by shift-clicking)

I made the Camera class and installed it in every character class like each robot installed
a pair of eyes. So theoretically we can use any character as the main character just focus her/his camera.
To test this camera on each character, I made a second small monitor screen function to monitor
the character I want to watch. The small monitor screen actually use the monitored character's camera viewport.

To make the second screen appear/disaapear more smooth, I built a finite state machine to control
the small monitor screen appearing/disaapearing process. And based on this I also built the realtime
small map system to let the user look at the whole map situation.

Beacuse the story is for 4 characters I add 4 character control mode and a function and an animation
to switch back/forth of 1 character/ 4 character mode. 4 character mode need more extra code to implement
each character's fight for route positions or yield route if he/she loses the position or wait for route if
the only route is occupied by others. And I also inplement switch animation and sound effect to make the switch more fun.

The other improvement is I rewrite the Map system, now the map system is
stored as a undirected graph, the adding map node is very easy at the initial time.
After initialization the map with necessary data information, the map node will
automatically incorporate with the whole map graph. And it automatically carries
the auto map switch function, map background music switch, foot step sound effeect switch.
The character class sotre's the shoe type and link the map node to make the same shoe has
different foot step sound due to the ground material changed as map change.
And there is a map node path finding function with DFS in Undirected graph algorithm.


The third improvement is adding the sound system. By using XAudio2 library, the game can
provide basic background music, sound effect and 3D positional sound. The foot step is 3D
positional sound and you can hear and feel the character moves with sound changes.

The fourth improvement is modularize the timer as class with standard c time library.
Now the animation class, walkingengine, FPS calculation, text display, notice display etc
all has animation effect function now can incorporate this timer class.

And a lot of small features are implemented such as versatile text display notice queue class,
mouse wheel control volume, mouse wheel control reverb effect, debug display etc.

The future work, the dialogue and story part is not that generic as a module. The dialoge pictures
are just static pre-made pictures with text in picture editor software. In the future,
this dialogue at least will be dynamic assembly as game running with text display by the program.

The turn-based battle system is not finished, when I have a time, I will finish this first.

If I can made some 3D model and 3D model animation data, I will try to make it as 3D.

Credit:

Sanqing Yuan giving me a lot of math and algorithm 
suggestions when I implemented mouse control. He and me made
the whole campus maps and menus buttons pictures.

Thanks for the classmates 2 years ago made the effects to this game in initial version:
Jason Alves-Foss made the character sprites and character models
Arian Norris  and Ian Hill designed the story, 
quests, character attributes and level system.
Fabian Mathijssen direct and orgnazie the whole game in the group.


The whole demo and introduction video is on youtube: https://youtu.be/8XbZs4j8Pi8


