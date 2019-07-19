## CleverPet Games

The [CleverPet Hub][cleverpet.io] is a programmable device that lets you
automatically train and interact with other species. It's kind of like a "game
console for pets".

<p align="center"> <img width="460" src="docs/images/hub1.png"> </p>

The Hub is built on the [Particle][particle.io] platform, which means that the
full suite of tools Particle has built can be used to control the CleverPet Hub.

This package shows you how to access the CleverPet Hub's HubInterface class,
letting you control the Hub's lights, food pod, and tray motor.

## How to install


## Including reporting


## What now?


## Definitions

In the hackerpet library words such as "challenge", "interaction" etc. are used
in specific ways:

*  **Player:** Any dog, cat, person, or other animal who is playing with a Hub.

*  **Foodtreat:** A food reward. E.g., a dog treat, cat treat, or piece of
   kibble.

*  **Report:** A single row of data describing everything that a player did
   during an interaction.

*  **Interaction:** A presentation of lights, sounds from a Hub, and the
   corresponding responses of a player, ending with a report. Nearly always, an
   interaction begins with the Hub doing some things, the player doing some
   things in response, and then the player getting some feedback as to whether
   they did the right thing.

*  **Challenge:** A series of one or more interactions, usually of progressively
   increasing difficulty, and often designed to teach the player a particular
   skill. *Example: the Responding Quickly challenge where the pet has to go
   through several iterations of pushing multiple lit up buttons and getting
   foodtreats.*

*  **Challenge set:** A series of challenges, such as the collection of 13
   original CleverPet challenges.

*  **Level:** A stage of difficulty within a given challenge. Lower levels are
   easier, and each challenge usually has a fixed number of them.

*  **Game:** A fuzzy term, currently without precise technical definition,
   sometimes used interchangeably with "challenge", but which may consist of
   multiple challenges.

## Contributing

This is where things can get really fun. Want to add a new game, and perhaps see
how the community, or dogs, or cats, respond to it? Create your own fork and
submit a pull request. We'll chat about it as a community, and if it seems
sensible we'll add it to the collection of games!

To make this work, all contributors first have to sign the [CleverPet Individual
Contributor License Agreement (CLA)][CLA], which is based on the Google CLA.
This agreement provides the CleverPet team with a license to re-distribute your
contributions.

Whenever possible, please follow these contribution guidelines:
- Keep each pull request small and focused on a single game, feature, or bug
  fix.
- Familiarize yourself with the code base, and follow the formatting principles
  adhered to in the surrounding code.

[hub]: https://github.com/CleverPet/HackerPet/blob/master/docs/images/hub1.png "Hackerpet hub"
[cleverpet.io]: https://clever.pet/ "CleverPet website"
[particle.io]: https://particle.io/ "Particle website"
[CLA]: https://docs.google.com/forms/d/e/1FAIpQLSeXAajtFZpQ0VtHK2APtfzrA5w8DMNagJhCfLVr6h9lCQgj1g/viewform "Contributor License Agreement"
