from abc import ABC, abstractmethod
from typing import List, Dict, Union

class Puppet(ABC):

    @abstractmethod
    def __init__(self):
        if not hasattr(self, 'version'):
            raise NotImplementedError("Puppet subclass must have a version attribute. This defines the version of the weights the current run is using.")
        
    @abstractmethod
    def setup_data(self, weights_path: str) -> None:
        """
        This function sets up the data for the model/experiment that is about to be trained. This functino should be called before any other function is called.
        This function can also be called to reset the data for the model/experiment.
        """
        pass
    
    @abstractmethod
    def train(self, epochs: int):
        """
        This function trains the model using the data that is stored in the model's internal state. It assumes that load_data has been called before this function.
        This function trains the model for the specified number of epochs.
        The specific puppet should keep track of the number of epochs it has been trained for.
        The epochs parameter here just represents how many additional epochs the model should be trained for.
        """
        pass

    @abstractmethod
    def validation_test(self):
        """
        This is a function does any 'validation' testing for the machine learning program.
        In the case of the poker bot, the 'validation testing' would be testing the exploitability of the bot.
        This function should also take care of saving any results from the validation testing.
        """
        pass

    @abstractmethod
    def move(self, board: List[str], hand: List[str], betting_history: List[str]) -> str:
        """
        This function takes in the current state of the game (board, hand, betting_history) and returns the bot's action. It assumes that load_data has been called before this function.

        Args:
            board (List[str]): The cards on the board.
            hand (str): The bot's hand.
            betting_history (List[str]): The betting history of the game.

        Returns:
            str: The bot's action.
        """
        pass

    @abstractmethod
    def get_pdf(self, board: List[str], hand: List[str], betting_history: List[str]) -> Dict[Union[float, str], float]:
        """
        This function takes in the current state of the game (board, hand, betting_history) and returns the probability distribution of the bot's actions. It assumes that load_data has been called before this function.
        """
    
    @abstractmethod
    def load_data(self, weights_path: str) -> None:
        """
        This function loads the data from the weights_path and stores it in the model's internal state. This essentially deserializes the data.
        """
        pass

    @abstractmethod
    def write_data(self, weights_folder: str) -> None:
        """
        This function writes the data from the model's internal state to the weights_folder. This essentially serializes the data.
        """
        pass
