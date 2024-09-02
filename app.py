from flask import Flask, render_template, request, redirect, url_for
import os
import json
import random
import string
from puppets.bangkok import Bangkok  # Import the Bangkok class
from conf.config_game import ConfigGame  # Assuming you have this imported from your conf
import sqlite3

app = Flask(__name__)

# Get the directory of the current file
current_file_directory = os.path.dirname(os.path.abspath(__file__))

# Load JSON data as a dictionary
def load_json_data(file_path):
    with open(file_path) as f:
        return json.load(f)
    
def update_experiment_column(db_path: str, experiment_name: str, column_name: str, new_value: str) -> None:
    """
    Update a specific column in the experiments table for a given experiment name.

    Args:
        db_path (str): The path to the SQLite database file.
        experiment_name (str): The name of the experiment to update.
        column_name (str): The column name to update.
        new_value (str): The new value to set for the specified column.
    """
    # Connect to the SQLite database
    connection = sqlite3.connect(db_path)
    cursor = connection.cursor()

    # Prepare the SQL query to update the specific column
    query = f"UPDATE experiments SET {column_name} = ? WHERE name = ?"

    try:
        # Execute the SQL query
        cursor.execute(query, (new_value, experiment_name))
        # Commit the changes
        connection.commit()
        print(f"Updated {column_name} for experiment '{experiment_name}' successfully.")
    except sqlite3.Error as e:
        print(f"An error occurred: {e}")
    finally:
        # Close the connection
        connection.close()

# Reconnect to the database
connection = sqlite3.connect('experiments.db')
cursor = connection.cursor()

# Retrieve all experiments
cursor.execute('SELECT * FROM experiments')
rows = cursor.fetchall()

experiments_data = {"experiments": {}}
# Process the results
for row in rows:
    name = row[0]
    puppet = row[1]
    description = row[2]
    status = row[3]
    epoch = row[4]
    data_loaded = row[5]
    created_at = row[6]
    updated_at = row[7]
    args = json.loads(row[8])  # Deserialize the JSON string back into a dictionary

    # print(data_loaded)
    # print(f'Experiment {name} with args: {args}')

    experiments_data["experiments"][name] = {
        "name": name,
        "puppet": puppet,
        "description": description,
        "status": status,
        "epoch": epoch,
        "data_loaded": bool(data_loaded),
        "created_at": created_at,
        "updated_at": updated_at,
        "args": args
    }
    
# Close the connection
connection.close()

# Load puppet configuration
puppet_config = load_json_data('conf/puppets.json')

# Initialize experiments data
for experiment_name in experiments_data["experiments"]:
    experiments_data["experiments"][experiment_name]["data_loaded"] = False
    update_experiment_column('experiments.db', experiment_name, "data_loaded", False)

# ensure all experiments start off as off
for experiment in experiments_data["experiments"]:
    experiments_data["experiments"][experiment]["status"] = "off"
    update_experiment_column('experiments.db', experiment, "status", "off")

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

        db_path = 'experiments.db'
        update_experiment_column(db_path, experiment_name, "data_loaded", True)

    return redirect(url_for('home'))

@app.route('/update_experiment/<experiment_name>/<param>', methods=['POST'])
def update_experiment(experiment_name, param):
    # Get the new value from the form
    new_value = request.form.get('new_value')
    
    # Convert new_value to the appropriate type
    new_value = int(new_value) if new_value.isdigit() else float(new_value)

    # Update the JSON data with the new value
    if experiment_name in experiments_data["experiments"]:
        experiments_data["experiments"][experiment_name]["args"][param] = new_value

        db_path = 'experiments.db'
        update_experiment_column(db_path, experiment_name, "args", json.dumps(experiments_data["experiments"][experiment_name]["args"]))

        # Also update the Bangkok model object if it exists
        model = experiment_to_model_obj.get(experiment_name)
        if model:
            setattr(model, param, new_value)

    return redirect(url_for('home'))

if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', port="5001")
