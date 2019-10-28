# symon

Symon is a version of the [Simon memory game by Milton-Bradley](https://en.wikipedia.org/wiki/Simon_(game)), but for dogs, cats, pigs, chimps, and any other animal that can be taught to play it.

![Original Milton-Bradley Simon game](https://upload.wikimedia.org/wikipedia/commons/thumb/c/cd/Simon_Electronic_Game.jpg/128px-Simon_Electronic_Game.jpg "Simon game")

## Game basics

One way of thinking about this game is as though it's "Monkey see, monkey do":

1. To begin with, it starts with all three touchpads illuminated. The player can touch any of them to start the game. 
2. **See**: A round begins with a single "knock" sound, followed by one or more touchpads lighting up in a sequence, along with the touchpad's associated sound.
3. **Do**: After the sequence has completed, a "double knock" sound will indicate that it's the player's turn to repeat it.

After the game has started, the three illuminated pads of Step 1 will only appear if the player doesn't respond. Otherwise, the game will continue immediately after the player's response, whether incorrect or correct. 

## Differences from MB Simon 

* The game doesn't immediately increase its difficulty by adding to the sequence. It trains players to play the game by first making sure players can be successful with a given sequence length.
* There are only three touchpads, rather than four. And the touchpads are all the same color.
* You can capture your player's learning data by using hackerpet's reporting functionality.

## Levels

In Symon, the game starts at Level 10 -- the game is designed to follow Challenge 9: "Learning Longer Sequences". 

Levels 10-19 involve having your player remember sequences that involve one light. 

Levels 20-29 involve your player remembering sequences that involve two lights. 

Levels 30-39 require remembering a sequence of three lights, and so. In general, it's possible to tell how long a sequence is that a player is working on by looking at the digit in the 10s place in the level number. Thus, as the levels go up, the game becomes more challenging. 

While working on a given sequence length, x0 through x9 change the difficulty of the game to help train your player. Two aspects of the game get modified to improve the player's training: 
1. "Exit on Miss probability" -- as the game shifts from x0 to x9, the game becomes more "strict", tolerating fewer incorrect touches as the player guesses which touchpad is the right one. We describe this as an increased probability of exiting the round when a player gets a touch wrong. 
2. "Delay to See phase" -- as the game shifts from x0 to x9, the maximum possible delay before the "see" phase begins increases, requiring the player to wait longer. The shortest possible delay is 50 ms, with the longest possible one being 2300 ms. 

Details on how the game advances and becomes more difficult can be found on the [Symon Game Description spreadsheet](https://docs.google.com/spreadsheets/d/1HkkUL4kADE9z8QU52XV4l3LqH5V2IuMB-91EWtAIcko/edit#gid=0).
<!-- Not implemented yet 
3. "Game switch probability" -- x0 through x5 involve practicing and practicing on the same sequence. In x6 through x9, the game will switch more and more often between sequences.
-->



