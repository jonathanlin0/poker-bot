from abc import ABC, abstractmethod
from typing import List

class Puppet(ABC):

    @abstractmethod
    def __init__(self):
        if not hasattr(self, 'version'):
            raise NotImplementedError("Puppet subclass must have a version attribute. This defines the version of the weights the current run is using.")
        
    @abstractmethod
    def train(self):
        """
        This function trains the model using the data that is stored in the model's internal state. It assumes that load_data has been called before this function.
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
    def get_pdf(self, board: List[str], hand: List[str], betting_history: List[str]) -> List[float]:
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
