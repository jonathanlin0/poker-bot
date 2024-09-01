from dataclasses import dataclass, field
import json

@dataclass
class ConfigGame:
    """
    The configuration for the game - they are essentially the rules of the modified poker game. 
    These values generally don't change.
    """

    # the maximum number of raises allowed per betting street
    # limited to shrink game tree and reduce training time
    max_raises_per_round: int = None
    # the stack size (in BB) of each player at the start of the game
    starting_stack_size: int = None
    infoset_delim: str = None
    config_file: str = field(default="conf/game.json")  # Default file path or can be provided during instantiation

    def __post_init__(self):
        """Automatically load configuration from the specified file upon instantiation."""
        self.load_from_file(self.config_file)

    def load_from_file(self, file_path: str):
        """Load configuration from a local JSON file."""
        try:
            with open(file_path, 'r') as file:
                data = json.load(file)
                
                self.max_raises_per_round = data["max_raises_per_round"]
                self.starting_stack_size = data["starting_stack_size"]
                self.infoset_delim = data["infoset_delim"]
        except Exception as e:
            print(f"An error occurred while loading configuration: {e}")
