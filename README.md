# sysprak

This is the code for a 10-week programming project I had to do in my third Bachelor semester ("Systempraktikum"), together with two friends.

The task was to implement a client that connects to a university server and plays the board game *bashni*, a Russian variation of checkers.
The client initially connects to the server by exchanging messages according to a predefined protocol.
During the game itself, it receives information about the current state of the game (i.e. the opponent's latest move) from the server, chooses a valid next move and sends it back to the server.
After the deadline, the clients from all the groups were pitted against each other in 1-vs-1 matches, and ours came in 2nd place out of 30 contestants. Hooray! 🥳

The learning objectives were:
* obtaining practical experience in teamwork and programming (on a somewhat larger scale)
* implementing concepts from systems programming such as pipes, signals and shared memory
* learning to get along with a new programming language autonomously (since we had only received a very cursory introduction to C beforehand)
