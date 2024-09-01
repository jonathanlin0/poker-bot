from flask import Flask, render_template, request, redirect, url_for
import os
import json
import random
import string
from puppets.bangkok import Bangkok  # Import the Bangkok class
from conf.config_game import ConfigGame  # Assuming you have this imported from your conf

app = Flask(__name__)

# Get the directory of the current file
current_file_directory = os.path.dirname(os.path.abspath(__file__))

# Generate a random string of the specified length
def get_random_string(length):
    letters = string.ascii_lowercase
    return ''.join(random.choice(letters) for i in range(length))

# Load JSON data as a dictionary
def load_json_data(file_path):
    with open(file_path) as f:
        return json.load(f)

# Load puppet configuration
puppet_config = load_json_data('conf/puppets.json')

# Load experiments data
experiments_data = load_json_data('database.json')

# Initialize experiments data
for experiment in experiments_data["experiments"].values():
    experiment["data_loaded"] = False

# Initialize the dictionary for mapping experiment names to Bangkok model objects
experiment_to_model_obj = {}

# Create instances of the Bangkok model for each experiment
for experiment_name, experiment_info in experiments_data["experiments"].items():
    # Retrieve the arguments from the JSON data
    args = experiment_info["args"]
    config_game = ConfigGame()  # Assuming ConfigGame has default or required parameters

    # Create a Bangkok model object using the arguments from the JSON file
    bangkok_model = Bangkok(
        reward_fn=args["reward_fn"],
        update_interval_epochs=args["update_interval_epochs"],
        exploit_test_multiplier=args["exploit_test_multiplier"],
        num_exploit_hands=args["num_exploit_hands"],
        equities_folder=args["equities_folder"],
        config_game=config_game
    )

    # Store the model object in the dictionary
    experiment_to_model_obj[experiment_name] = bangkok_model

@app.route('/')
def home():
    experiments = experiments_data["experiments"]
    return render_template('index.html', experiments=experiments, puppet_config=puppet_config)

@app.route('/load_data/<experiment_name>', methods=['POST'])
def load_data(experiment_name):
    # Update the data_loaded value for the specific experiment
    if experiment_name in experiments_data["experiments"]:
        # Get the corresponding Bangkok model object
        model = experiment_to_model_obj.get(experiment_name)
        
        # Run the model's load_data function with the specified file path
        if model:
            model.load_data('data/bangkok_1')  # Load data from the specified path

        # Update the JSON data to reflect that data has been loaded
        experiments_data["experiments"][experiment_name]["data_loaded"] = True

        # Save the updated data back to the JSON file to persist changes
        with open('database.json', 'w') as f:
            json.dump(experiments_data, f, indent=4)

    return redirect(url_for('home'))

@app.route('/save_data/<experiment_name>', methods=['POST'])
def save_data(experiment_name):
    # Get the corresponding Bangkok model object
    model = experiment_to_model_obj.get(experiment_name)
    
    # Run the model's write_data function with the specified file path
    if model:
        model.write_data('data/bangkok_1')  # Save data to the specified path

    return redirect(url_for('home'))

@app.route('/update_experiment/<experiment_name>/<param>', methods=['POST'])
def update_experiment(experiment_name, param):
    # Get the new value from the form
    new_value = request.form.get('new_value')
    
    # Convert new_value to the appropriate type
    if param in ['update_interval_epochs', 'num_exploit_hands']:
        new_value = int(new_value)
    elif param == 'exploit_test_multiplier':
        new_value = float(new_value)

    # Update the JSON data with the new value
    if experiment_name in experiments_data["experiments"]:
        experiments_data["experiments"][experiment_name]["args"][param] = new_value

        # Save the updated data back to the JSON file to persist changes
        with open('database.json', 'w') as f:
            json.dump(experiments_data, f, indent=4)

        # Also update the Bangkok model object if it exists
        model = experiment_to_model_obj.get(experiment_name)
        if model:
            setattr(model, param, new_value)

    return redirect(url_for('home'))

if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', port="5001")
