from flask import Flask, render_template, request, redirect, url_for, jsonify
import os
import json
from puppets.bangkok import Bangkok
from conf.config_game import ConfigGame
import sqlite3
import subprocess
from util.db import DB

app = Flask(__name__)

# Get the directory of the current file
current_file_directory = os.path.dirname(os.path.abspath(__file__))

# Load JSON data as a dictionary
def load_json_data(file_path):
    with open(file_path) as f:
        return json.load(f)

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

# check that all the puppet types in the experiments have a class in puppets/ and a configuration exists for them
for experiment in experiments_data["experiments"]:
    if experiments_data["experiments"][experiment]["puppet"] not in puppet_config:
        print(f"Error: Puppet type '{experiments_data['experiments'][experiment]['puppet']}' not found in puppet configuration.")
        exit(1)
    
    puppet_class = experiments_data["experiments"][experiment]["puppet"].capitalize()
    puppet_module = f"puppets.{puppet_class.lower()}"
    try:
        exec(f"from {puppet_module} import {puppet_class}")
    except ImportError:
        print(f"Error: Puppet class '{puppet_class}' not found in module '{puppet_module}'.")
        exit(1)
print("All puppet classes and matching configurations found.")

# Initialize experiments data
for experiment_name in experiments_data["experiments"]:
    experiments_data["experiments"][experiment_name]["data_loaded"] = False
    DB.update_experiment_column('experiments.db', experiment_name, "data_loaded", False)

# ensure all experiments start off as off
for experiment in experiments_data["experiments"]:
    experiments_data["experiments"][experiment]["status"] = "off"
    DB.update_experiment_column('experiments.db', experiment, "status", "off")

# Initialize the dictionary for mapping experiment names to Bangkok model objects
experiment_to_model_obj = {}

# Create instances of the Bangkok model for each experiment
for experiment_name, experiment_info in experiments_data["experiments"].items():
    # Retrieve the arguments from the JSON data
    args = experiment_info["args"]
    config_game = ConfigGame()  # Assuming ConfigGame has default or required parameters

    # Create a Bangkok model object using the arguments from the database
    bangkok_model = Bangkok(**args, config_game=config_game)

    # Store the model object in the dictionary
    experiment_to_model_obj[experiment_name] = bangkok_model

@app.route('/')
def home():
    experiments = experiments_data["experiments"]
    return render_template('index.html', experiments=experiments, puppet_config=puppet_config)

@app.route('/reset/<experiment_name>', methods=['POST'])
def reset_experiment(experiment_name):
    if experiment_name in experiments_data["experiments"]:
        # Get the corresponding Bangkok model object
        model = experiment_to_model_obj.get(experiment_name)
        
        # Run the model's setup_data function with the specified file path
        if model:
            model.setup_data('data/bangkok_1')  # Load data from the specified path

        # reset the experiment's values
        experiments_data["experiments"][experiment_name]["epoch"] = 0
        experiments_data["experiments"][experiment_name]["status"] = "off"

        db_path = 'experiments.db'
        DB.update_experiment_column(db_path, experiment_name, "epoch", 0)
        DB.update_experiment_column(db_path, experiment_name, "status", "off")

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
        DB.update_experiment_column(db_path, experiment_name, "args", json.dumps(experiments_data["experiments"][experiment_name]["args"]))

        # Also update the Bangkok model object if it exists
        model = experiment_to_model_obj.get(experiment_name)
        if model:
            setattr(model, param, new_value)

    return redirect(url_for('home'))

# Keep track of running scripts and their processes
running_scripts = {}
processes = {}

@app.route('/train_experiment/<experiment_name>', methods=['POST'])
def train_experiment(experiment_name):
    """
    Triggers the training script for the specified experiment and prevents multiple runs.
    """
    # Define the path to the train.py script
    train_script_path = os.path.join(os.getcwd(), 'train.py')
    db_path = 'experiments.db'

    # Check if the script is already running for this experiment
    if running_scripts.get(experiment_name, False):
        # If running, do nothing and redirect to home
        return redirect(url_for('home'))

    # Mark the script as running
    running_scripts[experiment_name] = True

    # Set the status to 'training' at the start
    DB.update_experiment_column(db_path, experiment_name, "status", "training")
    experiments_data["experiments"][experiment_name]["status"] = "training"

    try:
        # Start the train.py script with the experiment name as an argument
        process = subprocess.Popen(['python', train_script_path, experiment_name])
        processes[experiment_name] = process  # Store the process object
        print(f"Training script started for experiment: {experiment_name}")
        process.wait()  # Wait for the script to finish
    except Exception as e:
        print(f"Error running training script: {e}")
    finally:
        # After the script completes or if an error occurs, set the status to 'off'
        DB.update_experiment_column(db_path, experiment_name, "status", "off")
        experiments_data["experiments"][experiment_name]["status"] = "off"
        # Mark the script as not running and remove the process object
        running_scripts[experiment_name] = False
        processes.pop(experiment_name, None)

    return redirect(url_for('home'))

@app.route('/stop_experiment/<experiment_name>', methods=['POST'])
def stop_experiment(experiment_name):
    """
    Stops the running training script for the specified experiment.
    """
    # Check if a process is running for the experiment
    if experiment_name in processes:
        process = processes[experiment_name]
        process.terminate()  # Terminate the process
        process.wait()  # Wait for the process to terminate
        processes.pop(experiment_name, None)  # Remove the process from the dictionary
        running_scripts[experiment_name] = False  # Update the running state

        # Set the status to 'off'
        db_path = 'experiments.db'
        DB.update_experiment_column(db_path, experiment_name, "status", "off")
        experiments_data["experiments"][experiment_name]["status"] = "off"

    return redirect(url_for('home'))

@app.route('/check_running_status', methods=['GET'])
def check_running_status():
    """
    Returns the running state of all experiments in JSON format.
    """
    return jsonify(running_scripts)

@app.route('/check_status', methods=['GET'])
def check_status():
    """
    Returns the current status of all experiments in JSON format.
    """
    # Reconnect to the database to fetch the latest status
    connection = sqlite3.connect('experiments.db')
    cursor = connection.cursor()

    # Retrieve the latest status of all experiments
    cursor.execute('SELECT name, status FROM experiments')
    rows = cursor.fetchall()

    # Prepare the status data
    status_data = {name: status for name, status in rows}
    
    # Close the connection
    connection.close()

    return jsonify(status_data)

if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', port="5001")
