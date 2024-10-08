# Poker Bot
### Jonathan Lin
January 2024 - Present

Play against the bot: [alwayspunting.com](https://www.alwayspunting.com)

Research Paper: [link](https://jonathanlin0.github.io/files/poker-bot-paper.pdf)

## Purpose
This repository contains the relevant code for a suite of tools relevant to my poker bot.

## Getting Started

#### Environment Variables
The bot requires the following environment variables to be set. This can be done by directly creating a `.env` file in the root directory of the project or using your IDE's environment variable settings.


| Environment Variable | Description                                 | Example Value       |
|----------------------|---------------------------------------------|---------------------|
| `MONGO_DB_PASSWORD`  | Your password for your MongoDB account. Used to connect to your cluster.    | `1srLPLrvK0pd08OX`  |

#### Running the Bot
Running the bot requires a few settings. Here are the following settings and descriptions.
| Setting                | Description                                                                 | Example Value       |
|------------------------|-----------------------------------------------------------------------------|---------------------|
| Reward Function        | The reward function for the bot. This maps how many chips are won or lost for regret learning. | Example: `linear` is 1.5 chips lost for every 1 chip. This setting would be used to create a tighter bot than if the `default` function was used |
| Save Weights Interval  | The interval in which the weights are saved.                                 | Example: 1000 (weights are saved every 1000 iterations) |
| Additional Notes       | Additional notes that may be useful.                                        | Example: Make sure to adjust the reward function based on desired bot behavior. |



## Exclusions
Currently, the bot does not contain `multithread.py`. This is the main file for training the poker bot. My training algorithm is proprietary, and I am not prepared to release it to the public at this time. If you would like to know more about the training algorithm, please contact me directly at `jonathan@caltech.edu`. 
